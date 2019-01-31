#include "io_threads.hpp"

namespace asio_fiber {

ContextThreads::ContextThreads(context_ptr ctx_)
    : ctx(ctx_), fake_work(ctx->get_executor()) {}
void ContextThreads::start(std::size_t thread_count = 1) {
  ctx->restart();
  for (std::size_t i = 0; i < thread_count; ++i) {
    threads.push_back(std::thread([this] {
      // auto count =
      ctx->run();
      // TODO: Use logger for these output.
      // std::cout << "IO Thread RUN " << count
      //           << " Times, Exit: " << std::this_thread::get_id() <<
      //           std::endl;
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