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
#include "thread_name.hpp"
#include "use_fiber_future.hpp"

using namespace boost::asio;
using namespace asio_fiber;

// Define the buffer size for each session.
const std::size_t session_buffer_size = 2048;

template <typename T>
void clean_shutdown_socket(std::shared_ptr<T> this_socket) {
  boost::system::error_code ec;
  this_socket->shutdown(socket_base::shutdown_both, ec);
  this_socket->close(ec);
}

int main(int argc, char const *argv[]) {
  context_ptr ctx = std::make_shared<boost::asio::io_context>();
  this_thread_name::set("main");
  auto ct = ContextThreads(ctx);
  // auto &ft = FiberThreads<boost::fibers::algo::work_stealing>::instance();
  // auto &ft = FiberThreads<boost::fibers::algo::shared_work>::instance();
  auto &ft = FiberThreads<boost::fibers::algo::round_robin>::instance();
  // auto &ft = FiberThreads<>::instance();
  auto thread_count = std::thread::hardware_concurrency();

  // Start fiber threads (Default included main thread)
  ft.init(thread_count);
  
  // Start io context thread.
  ct.start(thread_count);

  // Init service here.
  boost::fibers::fiber(
      // Use dispatch to let fibers run asap.
      boost::fibers::launch::post,
      [ctx, &ft] {
        BOOST_LOG_TRIVIAL(info)
            << "Start ECHO TCP Server:" << boost::this_fiber::get_id();

        ip::tcp::acceptor acceptor(*ctx,
                                   ip::tcp::endpoint(ip::tcp::v4(), 10495));
        while (true) {
          auto this_socket =
              std::make_shared<boost::asio::ip::tcp::socket>(*ctx);

          // Accept for new connect...
          acceptor.async_accept(*this_socket, boost::asio::fibers::use_future)
              .get();  // ... and wait here.

          ft.post([this_socket, &ft] {
            // New fiber for this socket
            boost::fibers::fiber(
                boost::fibers::launch::post,
                [this_socket, &ft] {
                  BOOST_LOG_TRIVIAL(info)
                      << "Start ECHO Session:" << boost::this_fiber::get_id()
                      << " in thread: " << this_thread_name::get();
                  std::vector<char> buffer(session_buffer_size);
                  auto last_active_time =
                      std::make_shared<std::chrono::steady_clock::time_point>(
                          std::chrono::steady_clock::now());

                  // ft.post([this_socket, last_active_time]
                  {  // Process client timeout.
                    std::weak_ptr<boost::asio::ip::tcp::socket> weak_socket =
                        this_socket;
                    boost::fibers::fiber(
                        boost::fibers::launch::post,
                        [weak_socket, last_active_time] {
                          // Timer did not need hold the socket
                          // boost::this_fiber::sleep_for(std::chrono::seconds(10));
                          boost::this_fiber::yield();
                          while (true) {
                            auto timeout =
                                *last_active_time + std::chrono::seconds(60);
                            {
                              auto this_socket = weak_socket.lock();
                              if (!this_socket) break;

                              auto now = std::chrono::steady_clock::now();
                              if (now > timeout) {
                                // Timeup! Close!
                                BOOST_LOG_TRIVIAL(info)
                                    << "ECHO Session timeout:"
                                    << boost::this_fiber::get_id() << "@"
                                    << this_thread_name::get();

                                clean_shutdown_socket(this_socket);

                                break;  // And timeout fiber done!
                              }
                            }

                            boost::this_fiber::sleep_until(timeout);
                          }
                        })
                        .detach();
                  }  //);

                  try {
                    while (true) {
                      auto read_future = this_socket->async_read_some(
                          boost::asio::buffer(&buffer[0], buffer.size()),
                          boost::asio::fibers::use_future);

                      auto read_size = read_future.get();  // Fiber yiled here.

                      // When fiber weakup, log the active time.
                      *last_active_time = std::chrono::steady_clock::now();

                      BOOST_LOG_TRIVIAL(debug)
                          << boost::this_fiber::get_id() << "@"
                          << this_thread_name::get() << "  Read:" << read_size;

                      auto write_future = boost::asio::async_write(
                          *this_socket,
                          boost::asio::buffer(&buffer[0], read_size),
                          boost::asio::fibers::use_future);

                      BOOST_LOG_TRIVIAL(debug)
                          << boost::this_fiber::get_id() << "@"
                          << this_thread_name::get() << " Write:"
                          << write_future.get();  // Fiber yiled here.
                    }
                  } catch (std::exception const &e) {
                    BOOST_LOG_TRIVIAL(warning)
                        << "Session: " << boost::this_fiber::get_id()
                        << " Error: " << e.what();
                  }

                  BOOST_LOG_TRIVIAL(info)
                      << "Stop ECHO Session:" << boost::this_fiber::get_id()
                      << " in thread: " << this_thread_name::get();
                  clean_shutdown_socket(this_socket);
                })
                .detach();  // Don't forget to detach fibers or it will stop
                            // run.
          });
        }
      })
      .detach();

  // Wait for stop
  ft.join();
  ct.stop();
  return EXIT_SUCCESS;
}
