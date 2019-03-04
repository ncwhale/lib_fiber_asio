#ifndef THREAD_NAME_HPP
#define THREAD_NAME_HPP

#include <string>
#include <boost/config.hpp>

#ifdef BOOST_HAS_PTHREADS
  #include <pthread.h>
#endif

namespace this_thread_name {
  thread_local std::string this_thread_name;

  void set(const std::string& name) {
    this_thread_name = name;
#ifdef BOOST_HAS_PTHREADS
    pthread_setname_np(pthread_self(), this_thread_name.c_str());
#endif
  }

  const std::string& get() {
    return this_thread_name;
  }
}

#endif //THREAD_NAME_HPP