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

class TestService : public TMTService<TestRequest, TestResponse, char*>
{
  public:
    TestService(std::string port, int threadsNumber):
        TMTService<TestRequest, TestResponse, char*>
        ("0.0.0.0", port, threadsNumber, this->handle_request_, this->parse_request_ )
    {
    }

    int run(std::string error)
    {
        TMTService::run();
        return 0;
    }

    static int parse_request_(TestRequest& req, char* begin, char* end){
        while(begin != end) req.content.push_back(*begin++);
        return 0;
    }
    static int handle_request_(TestRequest& req, TestResponse& res){
        res.content = std::string("HTTP/1.1 200 OK\r\nServer: logrtbd\r\nContent-Length: 2");
        res.content+= std::string("\r\nContent-Type: text/plain\r\nConnection: Closed\r\n\r\nOK");
        return 0;
    }

    int parse_request(TestRequest& req, char* begin, char* end){
        req.capital_letters_count=0;
        while(begin != end){
            if(isupper(*begin)) req.capital_letters_count++;
            req.content.push_back(*begin++);
        }
        return 0;
    }

    int handle_request(TestRequest& req, TestResponse& res){
       req.content+="\nAnd there is "+std::to_string(req.capital_letters_count)+" capital letters in this text";
       res.content = std::string("HTTP/1.1 200 OK\r\nServer: logrtbd\r\nContent-Length: ")+std::to_string(req.content.size());
       res.content+= std::string("\r\nContent-Type: text/plain\r\nConnection: Closed\r\n\r\n")+req.content;
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
