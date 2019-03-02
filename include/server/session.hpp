#ifndef ASIO_FIBERS_IMPL_SESSION_HPP
#define ASIO_FIBERS_IMPL_SESSION_HPP

#include <boost/asio.hpp>
#include <boost/fiber/all.hpp>
#include <memory>

namespace asio_fiber {

using tcp = boost::asio::ip::tcp;

class session : public std::enable_shared_from_this<session> {
 public:
  session(tcp::socket socket) : is_ssl_(false), socket_(std::move(socket)) {}
  session(tcp::socket socket) : is_ssl_(true), socket_(std::move(socket)) {}

  void start() {}

 private:
  do_handleshake() {}

 private:
  bool is_ssl_;
  tcp::socket socket_;
};

class ssl_session : public std::enable_shared_from_this<ssl_session> {};
}  // namespace asio_fiber
#endif  // ASIO_FIBERS_IMPL_SESSION_HPP