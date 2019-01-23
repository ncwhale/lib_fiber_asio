#ifndef LIB_ASIO_FIBER_CONTEXT_THREADS
#define LIB_ASIO_FIBER_CONTEXT_THREADS

#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace asio_fiber {
typedef std::shared_ptr<boost::asio::io_context> context_ptr;
typedef boost::asio::executor_work_guard<boost::asio::io_context::executor_type> context_work;

class ContextThreads
{
public:
  ContextThreads(context_ptr ctx_) : ctx(ctx_) {}

  void start(std::size_t);
  void stop();

private:
  context_ptr ctx;
  std::vector<std::thread> threads;
};

void ContextThreads::start(std::size_t thread_count = 1)
{
  ctx->restart();
  for (std::size_t i = 0; i < thread_count; ++i)
  {
    threads.push_back(std::thread([this] { auto count = ctx->run();
    std::cout << "IO Thread RUN " << count << " Times, Exit: " << std::this_thread::get_id() << std::endl; }));
  }
}

void ContextThreads::stop()
{
  ctx->stop();
  for (std::thread &t : threads)
  {
    if (t.joinable())
      t.join();
  }
  threads.clear();
}
}

#endif // LIB_ASIO_FIBER_CONTEXT_THREADS
