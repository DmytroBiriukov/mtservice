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

class TMTService;


class TServiceConnection: public boost::enable_shared_from_this<TServiceConnection>,
        private boost::noncopyable
{
 public:
    explicit TServiceConnection(boost::asio::io_service& io_service,
                                TMTService* service
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
   /// Buffer for incoming data.
   boost::array<char, 8192> buffer_;   
   /// The reply to be sent back to the client.
   TBasicResponse response_;
   TMTService* service_;
};


class TMTService: private boost::noncopyable
{
 public:
  explicit TMTService(const std::string& address, const std::string& port, std::size_t thread_pool_size
                      );
  void run();
  virtual int handle_request(char* begin, char* end, TBasicResponse& res)=0;
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
  boost::shared_ptr<TServiceConnection> new_connection_;
};


#endif // MTSERVICE_HPP
