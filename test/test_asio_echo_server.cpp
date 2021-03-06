#include <boost/asio.hpp>
#include <chrono>
#include <memory>
#include <thread>  // for std::thread::hardware_concurrency()
#include <vector>
#include "fiber_threads.hpp"
#include "io_threads.hpp"
#include "use_fiber_future.hpp"

using namespace boost::asio;
using namespace asio_fiber;
const std::size_t session_buffer_size = 32;

int main(int argc, char const *argv[]) {
  context_ptr ctx = std::make_shared<boost::asio::io_context>();
  auto ct = ContextThreads(ctx);
  auto &ft = FiberThreads<>::instance();
  auto thread_count = std::thread::hardware_concurrency();

  // Start threads.
  ft.init(thread_count / 2);
  ct.start(thread_count / 2);

  // Init service here.
  boost::fibers::fiber(
      boost::fibers::launch::dispatch,
      [ctx] {
        std::cout << "Start ECHO TCP Server:" << boost::this_fiber::get_id()
                  << std::endl;

        ip::tcp::acceptor acceptor(*ctx,
                                   ip::tcp::endpoint(ip::tcp::v4(), 10495));
        while (true) {
          auto this_socket =
              std::make_shared<boost::asio::ip::tcp::socket>(*ctx);
          auto future = acceptor.async_accept(*this_socket,
                                              boost::asio::fibers::use_future);
          future.get();

          boost::fibers::fiber(
              boost::fibers::launch::dispatch,
              [this_socket] {
                std::cout << "Start ECHO Session:"
                          << boost::this_fiber::get_id() << std::endl;
                std::vector<char> buffer(session_buffer_size);
                try {
                  while (true) {
                    auto read_future = this_socket->async_read_some(
                        boost::asio::buffer(&buffer[0], buffer.size()),
                        boost::asio::fibers::use_future);

                    auto read_size = read_future.get();
                    std::cout << boost::this_fiber::get_id()
                              << " Read: " << read_size << std::endl;

                    auto write_future = boost::asio::async_write(
                        *this_socket,
                        boost::asio::buffer(&buffer[0], read_size),
                        boost::asio::fibers::use_future);

                    std::cout << boost::this_fiber::get_id()
                              << " Write :" << write_future.get() << std::endl;
                  }
                } catch (std::exception const &e) {
                  std::cout << "Session: " << boost::this_fiber::get_id()
                            << " Error: " << e.what() << std::endl;
                }

                std::cout << "Stop ECHO Session:" << boost::this_fiber::get_id()
                          << std::endl;
                this_socket->shutdown(socket_base::shutdown_both);
                this_socket->close();
              })
              .detach();
        }
      })
      .detach();

  // Wait for service stop.
  ft.join();
  ct.stop();
  return EXIT_SUCCESS;
}
