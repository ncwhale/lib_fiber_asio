//
// io_threads.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Whale Mo (ncwhale at gmail dot com)
//
#include "io_threads.hpp"
#include <sstream>
#include "thread_name.hpp"

namespace asio_fiber {

ContextThreads::ContextThreads(context_ptr ctx_)
    : ctx(ctx_), fake_work(new context_work(ctx->get_executor())) {}

void ContextThreads::start(std::size_t thread_count = 1) {
  ctx->restart();
  for (std::size_t i = 0; i < thread_count; ++i) {
    threads.push_back(std::thread([this, i] {
      {
        std::ostringstream oss;
        oss << "IO-Thread-" << i;
        this_thread_name::set(oss.str());
      }
      ctx->run();
    }));
  }
}

void ContextThreads::stop() {
  ctx->stop();
  for (std::thread &t : threads) {
    if (t.joinable()) t.join();
  }
  threads.clear();
}

}  // namespace asio_fiber
