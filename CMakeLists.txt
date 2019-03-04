cmake_minimum_required(VERSION 3.0.0)
project(asio_fibers VERSION 1.0.0)

include(CTest)
enable_testing()

set(Boost_DEBUG OFF)
find_package(Boost 1.68 REQUIRED
             COMPONENTS system
                        fiber
                        date_time
                        context)

if(Boost_FOUND)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include"
                      "${Boost_INCLUDE_DIRS}")

  add_library(asio_fibers STATIC "src/io_threads.cpp")

  target_include_directories(asio_fibers
                             PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

  set(CPACK_PROJECT_NAME ${PROJECT_NAME})
  set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
  include(CPack)

  add_executable(test_fiber_threads ./test/test_fiber_threads.cpp)
  target_link_libraries(test_fiber_threads asio_fibers ${Boost_LIBRARIES} pthread rt)

  add_executable(test_asio_timer ./test/test_asio_timer.cpp)
  target_link_libraries(test_asio_timer asio_fibers ${Boost_LIBRARIES} pthread rt)

  add_executable(test_echo_server ./test/test_echo_server.cpp)
  target_link_libraries(test_echo_server asio_fibers ${Boost_LIBRARIES} pthread rt)
endif()
