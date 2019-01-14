cmake_minimum_required(VERSION 3.0.0)
project(asio_fibers VERSION 0.1.0)

include(CTest)
enable_testing()

find_package(Boost 1.68 REQUIRED
             COMPONENTS iostreams
                        system
                        program_options
                        date_time
                        fiber
                        regex
                        context
                        thread
                        chrono
                        atomic)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
add_library(asio_fibers INTERFACE)
target_include_directories(asio_fibers
                           INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

add_executable(test_fiber_threads ./test/test_fiber_threads.cpp)
target_link_libraries(test_fiber_threads ${Boost_LIBRARIES} pthread rt)

add_executable(test_asio_timer ./test/test_asio_timer.cpp)
target_link_libraries(test_asio_timer ${Boost_LIBRARIES} pthread rt)

add_executable(test_af_timer ./test/test_fiber_asio_timer.cpp)
target_link_libraries(test_af_timer ${Boost_LIBRARIES} pthread rt)

add_executable(test_echo_server ./test/test_asio_echo_server.cpp)
target_link_libraries(test_echo_server ${Boost_LIBRARIES} pthread rt)
