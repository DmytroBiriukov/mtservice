#ifndef MTSERVICE_HPP
#define MTSERVICE_HPP

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <string>
#include <vector>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#include <functional>

class TBasicRequest{
 public:
    TBasicRequest(){
    }
    std::string content;
};

class TBasicResponse{
 public:
    TBasicResponse(){
    }
    std::string content;
    std::vector<boost::asio::const_buffer>to_buffers()
    {
      std::vector<boost::asio::const_buffer> buffers;
      buffers.push_back(boost::asio::buffer(content));
      return buffers;
    }
};
template <class TRequest, class TResponse, class TIterator>
class TMTService;

template <class TRequest, class TResponse, class TIterator>
class TServiceConnection: public boost::enable_shared_from_this<TServiceConnection <TRequest, TResponse, TIterator> >,
        private boost::noncopyable
{
 public:
    explicit TServiceConnection(boost::asio::io_service& io_service,
                                TMTService<TRequest, TResponse, TIterator>* service,
                       int (*ptr_handle_request)(TRequest& , TResponse& ),
                       int (*ptr_parse_request)(TRequest&, TIterator, TIterator)
                      );
    boost::asio::ip::tcp::socket& socket();
    /// Start the first asynchronous operation for the connection.
    void start();
 private:
   /// Handle completion of a read operation.
   void handle_read(const boost::system::error_code& e,
             std::size_t bytes_transferred);
   /// Handle completion of a write operation.
   void handle_write(const boost::system::error_code& e);
   /// Strand to ensure the connection's handlers are not called concurrently.
   boost::asio::io_service::strand strand_;
   /// Socket for the connection.
   boost::asio::ip::tcp::socket socket_;
   /// The handler used to process the incoming request.
   int (*ptr_handle_request_)(TRequest& req, TResponse& res);
   /// Buffer for incoming data.
   boost::array<char, 8192> buffer_;
   /// The incoming request.
   TRequest request_;
   /// The parser for the incoming request.
   int (*ptr_parse_request_)(TRequest& req, TIterator begin, TIterator end);
   /// The reply to be sent back to the client.
   TResponse response_;
   TMTService<TRequest, TResponse, TIterator>* service_;
};

template<class TRequest, class TResponse, class TIterator>
TServiceConnection<TRequest, TResponse, TIterator>::TServiceConnection(boost::asio::io_service& io_service,
                                                                       TMTService<TRequest, TResponse, TIterator>* service,
                                                                       int (*ptr_handle_request)(TRequest& , TResponse& ),
                                                                       int (*ptr_parse_request)(TRequest&, TIterator, TIterator)
                                       ): strand_(io_service), socket_(io_service)
{ ptr_handle_request_ = ptr_handle_request;
  ptr_parse_request_ = ptr_parse_request;
  service_ = service;
}

template <class TRequest, class TResponse, class TIterator>
boost::asio::ip::tcp::socket& TServiceConnection<TRequest, TResponse, TIterator>::socket()
{
  return socket_;
}
template <class TRequest, class TResponse, class TIterator>
void TServiceConnection<TRequest, TResponse, TIterator>::start()
{          socket_.async_read_some(boost::asio::buffer(buffer_),
              strand_.wrap(
                boost::bind(&TServiceConnection<TRequest, TResponse, TIterator>::handle_read,
                            this->shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred)));
}
template <class TRequest, class TResponse, class TIterator>
void TServiceConnection<TRequest, TResponse, TIterator>::handle_read(const boost::system::error_code& e, std::size_t bytes_transferred)
{ if(!e){
        // (1) Option 1 - use function pointers
            //ptr_parse_request_(request_, buffer_.data(), buffer_.data() + bytes_transferred);
            //ptr_handle_request_(request_, response_);
        // (2) Option 2 - prefferable - use overrided pure virtual functions
            service_->parse_request(request_, buffer_.data(), buffer_.data() + bytes_transferred);
            service_->handle_request(request_, response_);
            boost::asio::async_write(socket_, response_.to_buffers(),
                  strand_.wrap(
                    boost::bind(&TServiceConnection<TRequest, TResponse, TIterator>::handle_write,
                                this->shared_from_this(),
                                boost::asio::placeholders::error)));
  }
}
template <class TRequest, class TResponse, class TIterator>
void TServiceConnection<TRequest, TResponse, TIterator>::handle_write(const boost::system::error_code& e)
{
  if(!e){   boost::system::error_code ignored_ec;
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
  }
}
// [end] TServiceConnection methods


template <class TRequest, class TResponse, class TIterator>
class TMTService: private boost::noncopyable
{
 public:
  explicit TMTService(const std::string& address, const std::string& port, std::size_t thread_pool_size,
                      int (*ptr_handle_request)(TRequest& , TResponse& ),
                      int (*ptr_parse_request)(TRequest&, TIterator, TIterator));
  void run();
  virtual int handle_request(TRequest& req, TResponse& res)=0;
  virtual int parse_request(TRequest& req, TIterator begin, TIterator end)=0;
 private:
  /// Initiate an asynchronous accept operation.
  void start_accept();
  /// Handle completion of an asynchronous accept operation.
  void handle_accept(const boost::system::error_code& e);
  /// Handle a request to stop the server.
  void handle_stop();
  /// The number of threads that will call io_service::run().
  std::size_t thread_pool_size_;
  /// The io_service used to perform asynchronous operations.
  boost::asio::io_service io_service_;
  /// The signal_set is used to register for process termination notifications.
  boost::asio::signal_set signals_;
  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor acceptor_;
protected:
  /// The next connection to be accepted.
  boost::shared_ptr<TServiceConnection<TRequest, TResponse, TIterator>> new_connection_;
  /// The handler for all incoming requests.
  int (*ptr_handle_request_)(TRequest& req, TResponse& res);
  /// The parser for the incoming request.
  int (*ptr_parse_request_)(TRequest& req, TIterator begin, TIterator end);
};

template <class TRequest, class TResponse, class TIterator>
TMTService<TRequest, TResponse, TIterator>::TMTService(const std::string& address, const std::string& port, std::size_t thread_pool_size,
                                                       int (*ptr_handle_request)(TRequest& , TResponse& ),
                                                       int (*ptr_parse_request)(TRequest&, TIterator, TIterator) )
          : thread_pool_size_(thread_pool_size),
            signals_(io_service_),
            acceptor_(io_service_),
            new_connection_()
{
          //std::cout<<"Hello from mta_server constructor. Address : "<<address<<" port : "<<port;
          ptr_handle_request_ = ptr_handle_request;
          ptr_parse_request_ = ptr_parse_request;

          // Register to handle the signals that indicate when the server should exit.
          // It is safe to register for the same signal multiple times in a program,
          // provided all registration for the specified signal is made through Asio.
          signals_.add(SIGINT);
          signals_.add(SIGTERM);
        #if defined(SIGQUIT)
          signals_.add(SIGQUIT);
        #endif
          signals_.async_wait(boost::bind(&TMTService<TRequest, TResponse, TIterator>::handle_stop, this));
          // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
          boost::asio::ip::tcp::resolver resolver(io_service_);
          boost::asio::ip::tcp::resolver::query query(address, port);
          boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
          acceptor_.open(endpoint.protocol());
          acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
          acceptor_.bind(endpoint);
          acceptor_.listen();
          start_accept();
}
template <class TRequest, class TResponse, class TIterator>
void TMTService <TRequest, TResponse, TIterator>::run()
{  
          // Create a pool of threads to run all of the io_services.
          std::vector<boost::shared_ptr<boost::thread> > threads;
          for (std::size_t i = 0; i < thread_pool_size_; ++i)
          {
            boost::shared_ptr<boost::thread> thread(new boost::thread(
                  boost::bind(&boost::asio::io_service::run, &io_service_)));
            threads.push_back(thread);
          }

          // Wait for all threads in the pool to exit.
          for (std::size_t i = 0; i < threads.size(); ++i)
            threads[i]->join();
}
template <class TRequest, class TResponse, class TIterator>
void TMTService<TRequest, TResponse, TIterator>::start_accept()
{
          new_connection_.reset(new TServiceConnection<TRequest, TResponse, TIterator>
                                (io_service_, this, ptr_handle_request_, ptr_parse_request_));
          acceptor_.async_accept(new_connection_->socket(),
              boost::bind(&TMTService<TRequest, TResponse, TIterator>::handle_accept, this,
                boost::asio::placeholders::error));          
}
template <class TRequest, class TResponse, class TIterator>
void TMTService<TRequest, TResponse, TIterator>::handle_accept(const boost::system::error_code& e)
{
  if (!e){ new_connection_->start();
  }
  start_accept();
}
template <class TRequest, class TResponse, class TIterator>
void TMTService<TRequest, TResponse, TIterator>::handle_stop()
{
  io_service_.stop();
}
// [end] TMTService methods


template<class T>
void daemonize_service(pid_t &pid, const char* process_name, T& server)
{
    /* Fork off the parent process */
    pid = fork();
    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);
    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);
    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);
    /* Catch, ignore and handle signals */
    //TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    /* Fork off for the second time*/
    pid = fork();
    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);
    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);
    /* Set new file permissions */
    umask(0);
    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    if ((chdir("/")) < 0) {
     /* Log any failure here */
       exit(EXIT_FAILURE);
    }

      //Close out the standard file descriptors

            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }
    /* Open the log file */
    openlog(process_name, LOG_PID, LOG_DAEMON);

    std::string error;
    if(server.run(error)){
       exit(EXIT_FAILURE);
    }


}
#endif // MTSERVICE_HPP
