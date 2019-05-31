#ifndef ASIO_FIBER_DIVIDER_THREAD_HPP
#define ASIO_FIBER_DIVIDER_THREAD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <thread>

namespace asio_fiber {
std::size_t divider_thread(float percent = 1.0, std::size_t min = 1) {
  auto thread_count = std::thread::hardware_concurrency();
  thread_count *= percent;
  if (thread_count < min) return min;
  return thread_count;
}
}  // namespace asio_fiber

#endif //ASIO_FIBER_DIVIDER_THREAD_HPP