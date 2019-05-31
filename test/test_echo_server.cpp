//
// test_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Whale Mo (ncwhale at gmail dot com)
//
// Need define this to use boost::log.
#define BOOST_LOG_DYN_LINK 1
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <chrono>
#include <memory>
#include <thread>  // for std::thread::hardware_concurrency()
#include <vector>
#include "fiber_threads.hpp"
#include "io_threads.hpp"
#include "use_fiber_future.hpp"

using namespace boost::asio;
using namespace asio_fiber;

// Define the buffer size for each session.
const std::size_t session_buffer_size = 2048;

int main(int argc, char const *argv[]) {
  context_ptr ctx = std::make_shared<boost::asio::io_context>();
  auto ct = ContextThreads(ctx);
  auto &ft = FiberThreads<>::instance();
  auto thread_count = std::thread::hardware_concurrency();

  // Start fiber threads (Default included main thread)
  ft.init(thread_count);
  // Start io context thread.
  ct.start(thread_count);

  // Init service here.
  boost::fibers::fiber(
      // Use dispatch to let fibers run asap.
      boost::fibers::launch::dispatch,
      [ctx] {
        std::cout << "Start ECHO TCP Server:" << boost::this_fiber::get_id();

        ip::tcp::acceptor acceptor(*ctx,
                                   ip::tcp::endpoint(ip::tcp::v4(), 10495));
        while (true) {
          auto this_socket =
              std::make_shared<boost::asio::ip::tcp::socket>(*ctx);

          // Accept for new connect...
          acceptor.async_accept(*this_socket, boost::asio::fibers::use_future)
              .get();  // ... and wait here.

          // New fiber for this socket
          boost::fibers::fiber(
              boost::fibers::launch::dispatch,
              [this_socket] {
                BOOST_LOG_TRIVIAL(info)
                    << "Start ECHO Session:" << boost::this_fiber::get_id();
                std::vector<char> buffer(session_buffer_size);

                try {
                  while (true) {
                    auto read_future = this_socket->async_read_some(
                        boost::asio::buffer(&buffer[0], buffer.size()),
                        boost::asio::fibers::use_future);

                    auto read_size = read_future.get();  // Fiber yiled here.

                    BOOST_LOG_TRIVIAL(debug) << boost::this_fiber::get_id()
                                             << "  Read:" << read_size;

                    auto write_future = boost::asio::async_write(
                        *this_socket,
                        boost::asio::buffer(&buffer[0], read_size),
                        boost::asio::fibers::use_future);

                    BOOST_LOG_TRIVIAL(debug)
                        << boost::this_fiber::get_id() << " Write:"
                        << write_future.get();  // Fiber yiled here.
                  }
                } catch (std::exception const &e) {
                  BOOST_LOG_TRIVIAL(warning)
                      << "Session: " << boost::this_fiber::get_id()
                      << " Error: " << e.what();
                }

                BOOST_LOG_TRIVIAL(info)
                    << "Stop ECHO Session:" << boost::this_fiber::get_id();
                this_socket->shutdown(socket_base::shutdown_both);
              })
              .detach();  // Don't forget to detach fibers or it will stop run.
        }
      })
      .detach();

  // Wait for stop
  ft.join();
  ct.stop();
  return EXIT_SUCCESS;
}
