#include <boost/fiber/all.hpp>
#include <chrono>
#include <sstream>
#include "fiber_threads.hpp"

using namespace asio_fiber;

static std::size_t fiber_count{0};
static boost::fibers::mutex mtx_count{};
static boost::fibers::condition_variable_any cnd_count{};
typedef std::unique_lock<boost::fibers::mutex> lock_type;

void whatevah(char me) {
  try {
    std::thread::id my_thread =
        std::this_thread::get_id(); /*< get ID of initial thread >*/
    {
      std::ostringstream buffer;
      buffer << "fiber " << me << " started on thread " << my_thread << '\n';
      std::cout << buffer.str() << std::flush;
    }
    // boost::this_fiber::sleep_for(std::chrono::microseconds(1));
    for (unsigned i = 0; i < 100; ++i) { /*< loop ten times >*/
      boost::this_fiber::yield();        /*< yield to other fibers >*/
      std::thread::id new_thread =
          std::this_thread::get_id(); /*< get ID of current thread >*/
      if (new_thread !=
          my_thread) { /*< test if fiber was migrated to another thread >*/
        my_thread = new_thread;
        std::ostringstream buffer;
        buffer << "fiber " << me << " switched to thread " << my_thread << '\n';
        std::cout << buffer.str() << std::flush;
      }
    }
  } catch (...) {
  }

  lock_type lk(mtx_count);
  if (0 ==
      --fiber_count) { /*< Decrement fiber counter for each completed fiber. >*/
    lk.unlock();
    cnd_count.notify_all(); /*< Notify all fibers waiting on `cnd_count`. >*/
  }
}

int main(int argc, char const *argv[]) {
  auto &ft          = FiberThreads<>::instance();
  auto thread_count = std::thread::hardware_concurrency();
  ft.init(thread_count);

  for (char c : std::string("abcdefghijklmnopqrstuvwxyz")) {
    boost::fibers::fiber([c]() { whatevah(c); }).detach();
    ++fiber_count;
  }

  boost::fibers::fiber([] {
    {
      lock_type lk(mtx_count);
      cnd_count.wait(lk, []() { return 0 == fiber_count; });
    }

    FiberThreads<>::instance().notify_stop();
  })
      .detach();

  ft.join();
  return EXIT_SUCCESS;
}
