#ifndef FIBER_FRAME_CONTEXT_HPP
#define FIBER_FRAME_CONTEXT_HPP

#include <boost/fiber/all.hpp>
#include <mutex>
#include <thread>
#include <vector>
#include "thread_barrier.hpp"

class FiberThreads {
 public:
  static FiberThreads &instance();

  void init(std::size_t, bool);

  void notify_stop();

  void join();

 private:
  FiberThreads()                        = default;
  FiberThreads(const FiberThreads &rhs) = delete;
  FiberThreads(FiberThreads &&rhs)      = delete;

  FiberThreads &operator=(const FiberThreads &rhs) = delete;
  FiberThreads &operator=(FiberThreads &&rhs) = delete;

  bool running = false;
  std::size_t fiber_thread_count;
  std::mutex run_mtx;
  boost::fibers::condition_variable_any m_cnd_stop;
  std::vector<std::thread> m_threads;
};

FiberThreads &FiberThreads::instance() {
  static FiberThreads fts;
  return fts;
}

void FiberThreads::init(std::size_t count = 2, bool use_this_thread = true) {
  {  // Only init when not running.
    std::lock_guard<std::mutex> lk(run_mtx);
    if (running) return;
    running            = true;
    fiber_thread_count = count;
  }

  // At least we need 2 threads for work_stealing fiber algo.
  if (use_this_thread && fiber_thread_count < 2) {
    // Or just install round_robin for this (main) thread.
    boost::fibers::use_scheduling_algorithm<boost::fibers::algo::round_robin>();
    return;
  }

  thread_barrier b(fiber_thread_count);
  auto thread_fun = [&b, this]() {
    boost::fibers::use_scheduling_algorithm<boost::fibers::algo::work_stealing>(
        fiber_thread_count);
    b.wait();

    {  // Sync all threads.
      std::unique_lock<std::mutex> lk(run_mtx);
      m_cnd_stop.wait(lk, [this]() { return !running; });
    }
  };

  for (int i = (use_this_thread ? 1 : 0); i < fiber_thread_count; ++i) {
    m_threads.push_back(std::thread(thread_fun));
  }

  if (use_this_thread) {
    boost::fibers::use_scheduling_algorithm<boost::fibers::algo::work_stealing>(
        fiber_thread_count, true);
    // sync use_scheduling_algorithm
    b.wait();
  }
}

void FiberThreads::notify_stop() {
  std::unique_lock<std::mutex> lk(run_mtx);
  running = false;
  lk.unlock();
  m_cnd_stop.notify_all();
}

void FiberThreads::join() {
  //检查结束条件
  {
    std::unique_lock<std::mutex> lk(run_mtx);
    m_cnd_stop.wait(lk, [this]() { return !running; });
  }

  for (std::thread &t : m_threads) {
    if (t.joinable()) t.join();
  }
}

#endif  // FIBER_FRAME_CONTEXT_HPP