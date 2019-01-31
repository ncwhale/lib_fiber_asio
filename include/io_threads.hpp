//
// io_threads.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Whale Mo (ncwhale at gmail dot com)
//
#ifndef LIB_ASIO_FIBER_CONTEXT_THREADS
#define LIB_ASIO_FIBER_CONTEXT_THREADS

#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace asio_fiber {
typedef std::shared_ptr<boost::asio::io_context> context_ptr;
typedef boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
    context_work;

class ContextThreads {
 public:
  ContextThreads(context_ptr ctx_);

  void start(std::size_t);
  void stop();

 private:
  context_ptr ctx;
  context_work fake_work;
  std::vector<std::thread> threads;
};

}  // namespace asio_fiber

#endif  // LIB_ASIO_FIBER_CONTEXT_THREADS
