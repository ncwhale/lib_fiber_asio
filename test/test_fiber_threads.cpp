//
// test_fiber_threads.cpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Whale Mo (ncwhale at gmail dot com)
//
#include <boost/fiber/all.hpp>
#include <chrono>
#include <sstream>
#include "fiber_threads.hpp"
#include "thread_name.hpp"

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
      buffer << "fiber " << me << " started on thread "
             << this_thread_name::get() << '\n';
      std::cout << buffer.str() << std::flush;
    }
    // boost::this_fiber::sleep_for(std::chrono::microseconds(1));
    for (unsigned i = 0; i < 100; ++i) { /*< loop 100 times >*/
      boost::this_fiber::yield();        /*< yield to other fibers >*/
      std::thread::id new_thread =
          std::this_thread::get_id(); /*< get ID of current thread >*/
      if (new_thread !=
          my_thread) { /*< test if fiber was migrated to another thread >*/
        my_thread = new_thread;
        std::ostringstream buffer;
        buffer << "fiber " << me << " switched to thread "
               << this_thread_name::get() << '\n';
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
  auto &ft = FiberThreads<>::instance();
  auto thread_count = std::thread::hardware_concurrency();
  ft.init(thread_count);

  auto start_point = std::chrono::steady_clock::now();
  {
    lock_type lk(mtx_count);
    for (char c : std::string("abcdefghijklmnopqrstuvwxyz")) {
      boost::fibers::fiber(boost::fibers::launch::post, [c]() { whatevah(c); })
          .detach();
      ++fiber_count;
    }
  }
  {
    std::ostringstream oss;
    oss << "Fibers init done! total :" << fiber_count << "\n";
    std::cout << oss.str() << std::flush;
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
  auto end_point = std::chrono::steady_clock::now();
  {  // Output process timing.
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_point - start_point);
    std::chrono::duration<double, std::milli> d_duration =
        end_point - start_point;
    std::ostringstream oss;
    oss << "Jobs done in " << duration.count() << "ms.\n";
    oss << "or " << d_duration.count() << "ms.";
    std::cout << oss.str() << std::endl;
  }
  return EXIT_SUCCESS;
}
