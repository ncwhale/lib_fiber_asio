#ifndef THREAD_NAME_HPP
#define THREAD_NAME_HPP

#include <string>

namespace this_thread_name {
  void set(const std::string& name);
  const std::string& get();
}

#endif //THREAD_NAME_HPP