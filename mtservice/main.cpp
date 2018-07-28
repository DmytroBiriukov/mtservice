#include <mtservice.hpp>
#include <string>

class TestRequest: public TBasicRequest{
 public:
    TestRequest(){
    }
    int capital_letters_count;
};

class TestResponse:public TBasicResponse{
 public:
    TestResponse(){}
};

class TestService : public TMTService
{
  public:
    TestService(std::string port, int threadsNumber):
        TMTService("0.0.0.0", port, threadsNumber)
    {
    }

    int run(std::string error)
    {
        TMTService::run();
        return 0;
    }

    int handle_request(char* begin, char* end, TBasicResponse& res){
       TestRequest test_req;
       TestResponse test_res;
       // parsing with some protocol requirements check
       // any special classes aka TRequest or TResponse are used inside handle_request exceptionally
       test_req.capital_letters_count=0;
       while(begin != end){
           if(isupper(*begin)) test_req.capital_letters_count++;
           test_req.content.push_back(*begin++);
       }
       // compile response
       // ...
       test_req.content+="\nAnd there is "+std::to_string(test_req.capital_letters_count)+" capital letters in this text";
       // fill response content
       res.content = std::string("HTTP/1.1 200 OK\r\nServer: logrtbd\r\nContent-Length: ")+std::to_string(test_req.content.size());
       res.content+= std::string("\r\nContent-Type: text/plain\r\nConnection: Closed\r\n\r\n")+test_req.content;
       return 0;
    }
};

int main(int argc, char *argv[])
{
    try
      { std::string port;
        std::string threadsNumber;
        if(argc > 2){
            port.assign(argv[1]);
            threadsNumber.assign(argv[2]);
        }else{
            port.assign("13000");
            threadsNumber.assign("128");
        }
        std::string error;
        TestService service_(port, std::stoi(threadsNumber));
        service_.run(error);
      }
      catch (std::exception& e)
      { std::cerr << "exception: " << e.what() << "\n";
      }
    return 0;
}
