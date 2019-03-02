//
// io_threads.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Whale Mo (ncwhale at gmail dot com)
//

#ifndef ASIO_FIBER_IO_THREADS
#define ASIO_FIBER_IO_THREADS

#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace asio_fiber {

typedef std::shared_ptr<boost::asio::io_context> context_ptr;
typedef boost::asio::executor_work_guard<boost::asio::io_context::executor_type> context_work;
typedef std::unique_ptr<context_work> fake_work_ptr;

class ContextThreads {
 public:
  ContextThreads(context_ptr ctx_);

  void start(std::size_t);
  void stop();

 private:
  context_ptr ctx;
  // A fake work will keep io_context run.
  fake_work_ptr fake_work;
  std::vector<std::thread> threads;
};

}  // namespace asio_fiber

#endif  // ASIO_FIBER_IO_THREADS
