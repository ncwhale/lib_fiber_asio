#include <boost/asio.hpp>
#include <chrono>
#include <memory>
#include <thread>  // for std::thread::hardware_concurrency()
#include "fiber_threads.hpp"
#include "io_threads.hpp"
#include "use_fiber_future.hpp"

using namespace boost::asio;
using namespace asio_fiber;
static std::size_t fiber_count{100};
static boost::fibers::mutex mtx_count{};
static boost::fibers::condition_variable_any cnd_count{};
typedef std::unique_lock<boost::fibers::mutex> lock_type;

int main(int argc, char const *argv[]) {
  context_ptr ctx   = std::make_shared<boost::asio::io_context>();
  auto ct           = ContextThreads(ctx);
  auto &ft          = FiberThreads<>::instance();
  auto thread_count = std::thread::hardware_concurrency();
  ft.init(thread_count);

  // Init service here.
  {
    // lock_type lk(mtx_count);

    for (int i = 0; i < fiber_count; ++i) {
      boost::fibers::fiber(
          boost::fibers::launch::dispatch,
          [ctx, &ct, thread_count] {
            auto thread_id = std::this_thread::get_id();
            std::cout << "Start timer:" << boost::this_fiber::get_id() << "@"
                      << thread_id << std::endl;
            auto timer = steady_timer(*ctx);
            timer.expires_after(chrono::seconds(1));
            auto future = timer.async_wait(boost::asio::fibers::use_future);
            // Must sync with main thread here.
            future.get();

            {
              lock_type lk(mtx_count);
              if (thread_id != std::this_thread::get_id())
                std::cout << "Fiber moved:" << boost::this_fiber::get_id()
                          << "@" << std::this_thread::get_id() << std::endl;
              if (0 == --fiber_count) { /*< Decrement fiber counter for each
                                           completed fiber. >*/
                lk.unlock();
                cnd_count.notify_all(); /*< Notify all fibers waiting on
                                           `cnd_count`. >*/
              }
            }
          })
          .detach();
    }
  }

  boost::fibers::fiber([] {
    // boost::this_fiber::sleep_for(std::chrono::seconds(2));
    boost::this_fiber::yield();
    {
      lock_type lk(mtx_count);
      cnd_count.wait(lk, []() { return 0 == fiber_count; });
    }
    FiberThreads<>::instance().notify_stop();
  })
      .detach();

  // Start io context thread.
  ct.start(thread_count);
  ft.join();
  ct.stop();
  return EXIT_SUCCESS;
}
