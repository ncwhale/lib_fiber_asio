#ifndef FIBER_FRAME_CONTEXT_HPP
#define FIBER_FRAME_CONTEXT_HPP

#include <boost/fiber/all.hpp>
#include <mutex>
#include <thread>
#include <vector>
#include "thread_barrier.hpp"

namespace asio_fiber {

template <typename fiber_scheduling_algorithm =
              boost::fibers::algo::shared_work>
class FiberThreads {
 public:
  static FiberThreads &instance();

  void init(std::size_t count = 2, bool use_this_thread = true,
            bool suspend_worker_thread = true);

  void notify_stop();

  void join();

 private:
  FiberThreads() = default;
  FiberThreads(const FiberThreads &rhs) = delete;
  FiberThreads(FiberThreads &&rhs) = delete;

  FiberThreads &operator=(const FiberThreads &rhs) = delete;
  FiberThreads &operator=(FiberThreads &&rhs) = delete;

  bool running = false;
  std::size_t fiber_thread_count;
  std::mutex run_mtx;
  boost::fibers::condition_variable_any m_cnd_stop;
  std::vector<std::thread> m_threads;
};

template <typename fiber_scheduling_algorithm>
void install_fiber_scheduling_algorithm(std::size_t thread_count,
                                        bool suspend) {
  // Default scheduling need zero param.
  boost::fibers::use_scheduling_algorithm<fiber_scheduling_algorithm>();
}

template <>
void install_fiber_scheduling_algorithm<boost::fibers::algo::shared_work>(
    std::size_t thread_count, bool suspend) {
  boost::fibers::use_scheduling_algorithm<boost::fibers::algo::shared_work>(
      suspend);
}

template <>
void install_fiber_scheduling_algorithm<boost::fibers::algo::work_stealing>(
    std::size_t thread_count, bool suspend) {
  boost::fibers::use_scheduling_algorithm<boost::fibers::algo::work_stealing>(
      thread_count, suspend);
}

template <typename fiber_scheduling_algorithm>
FiberThreads<fiber_scheduling_algorithm>
    &FiberThreads<fiber_scheduling_algorithm>::instance() {
  static FiberThreads<fiber_scheduling_algorithm> ft;
  return ft;
}

template <typename fiber_scheduling_algorithm>
void FiberThreads<fiber_scheduling_algorithm>::init(
    std::size_t count, bool use_this_thread, bool suspend_worker_thread) {
  // Check param for init.
  if (!use_this_thread and count < 1) {
    // TODO: throw expection?
    return;
  }

  {  // Only init when not running.
    std::lock_guard<std::mutex> lk(run_mtx);
    if (running) return;
    running = true;
    fiber_thread_count = count;
  }

  // At least we need 2 threads for other fiber algo.
  if (use_this_thread && fiber_thread_count < 2) {
    // Use round_robin for this (main) thread only.
    install_fiber_scheduling_algorithm<boost::fibers::algo::round_robin>(
        fiber_thread_count, suspend_worker_thread);
    return;
  }

  thread_barrier b(fiber_thread_count);
  auto thread_fun = [&b, this, suspend_worker_thread]() {
    install_fiber_scheduling_algorithm<fiber_scheduling_algorithm>(
        fiber_thread_count, suspend_worker_thread);

    // Sync all threads.
    b.wait();

    {  // Wait for fibers run.
      std::unique_lock<std::mutex> lk(run_mtx);
      m_cnd_stop.wait(lk, [this]() { return !running; });
    }
  };

  for (int i = (use_this_thread ? 1 : 0); i < fiber_thread_count; ++i) {
    m_threads.push_back(std::thread(thread_fun));
  }

  if (use_this_thread) {
    install_fiber_scheduling_algorithm<fiber_scheduling_algorithm>(
        fiber_thread_count, suspend_worker_thread);
    // sync with worker threads.
    b.wait();
  }
}

template <typename fiber_scheduling_algorithm>
void FiberThreads<fiber_scheduling_algorithm>::notify_stop() {
  std::unique_lock<std::mutex> lk(run_mtx);
  running = false;
  lk.unlock();
  m_cnd_stop.notify_all();
}

template <typename fiber_scheduling_algorithm>
void FiberThreads<fiber_scheduling_algorithm>::join() {
  //检查结束条件
  {
    std::unique_lock<std::mutex> lk(run_mtx);
    m_cnd_stop.wait(lk, [this]() { return !running; });
    std::cout << "Fibbers not running!" << std::endl;
  }

  for (std::thread &t : m_threads) {
    if (t.joinable()) t.join();
  }
}

}  // namespace asio_fiber

#endif  // FIBER_FRAME_CONTEXT_HPP
