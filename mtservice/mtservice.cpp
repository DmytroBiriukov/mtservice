#include <mtservice.hpp>

TServiceConnection::TServiceConnection(boost::asio::io_service& io_service, TMTService* service
                                       ): strand_(io_service), socket_(io_service)
{
  service_ = service;
}


boost::asio::ip::tcp::socket& TServiceConnection::socket()
{
  return socket_;
}

void TServiceConnection::start()
{          socket_.async_read_some(boost::asio::buffer(buffer_),
              strand_.wrap(
                boost::bind(&TServiceConnection::handle_read,
                            this->shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred)));
}

void TServiceConnection::handle_read(const boost::system::error_code& e, std::size_t bytes_transferred)
{ if(!e){

        // Option 3 - any specifics of TRequest and TResponse are implemented inside overrriden functions
        // TServiceConnection doesn't need to know these specifics
            service_->handle_request(buffer_.data(), buffer_.data() + bytes_transferred, response_);
            boost::asio::async_write(socket_, response_.to_buffers(),
                  strand_.wrap(
                    boost::bind(&TServiceConnection::handle_write,
                                this->shared_from_this(),
                                boost::asio::placeholders::error)));
  }
}

void TServiceConnection::handle_write(const boost::system::error_code& e)
{
  if(!e){   boost::system::error_code ignored_ec;
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
  }
}
// [end] TServiceConnection methods


TMTService::TMTService(const std::string& address, const std::string& port, std::size_t thread_pool_size
           )
          : thread_pool_size_(thread_pool_size),
            signals_(io_service_),
            acceptor_(io_service_),
            new_connection_()
{
          // Register to handle the signals that indicate when the server should exit.
          // It is safe to register for the same signal multiple times in a program,
          // provided all registration for the specified signal is made through Asio.
          signals_.add(SIGINT);
          signals_.add(SIGTERM);
        #if defined(SIGQUIT)
          signals_.add(SIGQUIT);
        #endif
          signals_.async_wait(boost::bind(&TMTService::handle_stop, this));
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

void TMTService::run()
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

void TMTService::start_accept()
{
          new_connection_.reset(new TServiceConnection
                                (io_service_, this));
          acceptor_.async_accept(new_connection_->socket(),
              boost::bind(&TMTService::handle_accept, this,
                boost::asio::placeholders::error));
}

void TMTService::handle_accept(const boost::system::error_code& e)
{
  if (!e){ new_connection_->start();
  }
  start_accept();
}

void TMTService::handle_stop()
{
  io_service_.stop();
}
// [end] TMTService methods

