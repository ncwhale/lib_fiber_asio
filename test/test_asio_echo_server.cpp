#include <boost/asio.hpp>
#include <chrono>
#include <memory>
#include <thread> // for std::thread::hardware_concurrency()
#include <vector>
#include "fiber_threads.hpp"
#include "io_threads.hpp"
#include "use_fiber_future.hpp"

using namespace boost::asio;
const std::size_t session_buffer_size = 2048;

int main(int argc, char const *argv[])
{
  context_ptr ctx = std::make_shared<boost::asio::io_context>();
  auto ct = ContextThreads(ctx);
  auto &ft = FiberThreads::instance();
  auto thread_count = std::thread::hardware_concurrency();
  ft.init(2);
  // ft.init(thread_count);

  // Init service here.

  boost::fibers::fiber(
      boost::fibers::launch::dispatch,
      [ctx] {
        std::cout << "Start ECHO TCP Server:" << boost::this_fiber::get_id()
                  << std::endl;

        ip::tcp::acceptor acceptor(*ctx,
                                   ip::tcp::endpoint(ip::tcp::v4(), 10495));
        while (true)
        {
          // boost::system::error_code ec;
          // typedef std::shared_ptr<boost::asio::ip::tcp::socket>
          // socket_ptr_type;
          auto this_socket =
              std::make_shared<boost::asio::ip::tcp::socket>(*ctx);
          auto future = acceptor.async_accept(*this_socket,
                                              boost::asio::fibers::use_future);
          future.get();
          // New fiber for this socket
          boost::fibers::fiber(
              boost::fibers::launch::dispatch,
              [this_socket] {
                std::cout << "Start ECHO Session:"
                          << boost::this_fiber::get_id() << std::endl;
                std::vector<char> buffer(session_buffer_size);

                try {
                  while (true) {
                    std::size_t read_size = 0;
                    auto read_future      = this_socket->async_read_some(
                        boost::asio::buffer(&buffer[0], buffer.size()),
                        boost::asio::fibers::use_future(
                            [&read_size](
                                boost::system::error_code ec,
                                std::size_t n) -> boost::system::error_code {
                              read_size = n;
                              return ec;
                            }));

                    read_future.get();
                    std::cout << boost::this_fiber::get_id()
                              << " Read: " << read_size << std::endl;

                    auto write_future = boost::asio::async_write(
                        *this_socket,
                        boost::asio::buffer(&buffer[0], read_size),
                        boost::asio::fibers::use_future);

                    write_future.get();

                    std::cout << boost::this_fiber::get_id() << " Write done"
                              << std::endl;
                  }
                } catch (std::exception const &e) {
                  std::cout << "Session: " << boost::this_fiber::get_id()
                            << " Error: " << e.what() << std::endl;
                }
                std::cout << "Stop ECHO Session:" << boost::this_fiber::get_id()
                          << std::endl;
                this_socket->shutdown(socket_base::shutdown_both);
              })
              .detach();
        }
      })
      .detach();

  // boost::fibers::fiber([] {
  //   // boost::this_fiber::sleep_for(std::chrono::seconds(2));
  //   FiberThreads::instance().notify_stop();
  // })
  //     .detach();
  // A fake work keep io_context run.
  typedef boost::asio::executor_work_guard<boost::asio::io_context::executor_type> io_context_work;
  std::unique_ptr<io_context_work> fake_work(new io_context_work(ctx->get_executor()));

  // Start io context thread.
  ct.start(1);
  // ct.start(thread_count);
  ft.join();
  ct.stop();
  return EXIT_SUCCESS;
}
