#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

typedef std::shared_ptr<boost::asio::io_context> context_ptr;

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