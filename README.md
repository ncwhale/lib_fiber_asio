# boost::fibber&asio library

A tiny library &amp; examples for join boost::fiber&amp;boost::asio toghter.

## How to use?

This example convert from asio use_future.

```CPP
void get_daytime(boost::asio::io_context& io_context, const char* hostname)
{
  try
  {
    udp::resolver resolver(io_context);

    boost::fibers::future<udp::resolver::results_type> endpoints =
      resolver.async_resolve(
          udp::v4(), hostname, "daytime",
          boost::asio::fibers::use_future);

    // The async_resolve operation above returns the endpoints as a future
    // value that is not retrieved ...

    udp::socket socket(io_context, udp::v4());

    std::array<char, 1> send_buf  = {{ 0 }};
    boost::fibers::future<std::size_t> send_length =
      socket.async_send_to(boost::asio::buffer(send_buf),
          *endpoints.get().begin(), // ... until here. This call may block.
          boost::asio::fibers::use_future);

    std::cout << "Endpoint get" << std::endl;

    // Do other things here while the send completes.

    std::cout << "Sended: " <<
    send_length.get() // Blocks until the send is complete. Throws any errors.
     << std::endl;

    std::array<char, 128> recv_buf;
    udp::endpoint sender_endpoint;
    boost::fibers::future<std::size_t> recv_length =
      socket.async_receive_from(
          boost::asio::buffer(recv_buf),
          sender_endpoint,
          boost::asio::fibers::use_future);

    // Do other things here while the receive completes.

    std::cout.write(
        recv_buf.data(),
        recv_length.get()); // Blocks until receive is complete.
  }
  catch (std::system_error& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

```

Then use it as a fiber:

```CPP
int main() {
    ...
    boost::fibers::fiber([&io_context, address]{
        get_daytime(io_context, address)
    });
    ...
}
```

## Design

This library works in this design:

* boost::asio runs in some threads;
* boost::fibers runs in other threads (May include the main thread);
* use `boost::asio::fibers::use_future` for every asio async call placeholder.
* the fibers will yield when `future.get` called, and weakup when value fullfill or exception occured.
* with threads helpers, these fibers & asio threads can be easy managed in main thread.

## Secial Thanks

These articals give me the idead to write this tiny library.


* [c++协程库boost.fiber库介绍(zh-CN)](https://zhuanlan.zhihu.com/p/39807017)

* [c++协程库boost.fiber的能力(zh-CN)](https://zhuanlan.zhihu.com/p/45665910)

* [fiber_kit](https://github.com/jxfwinter/fiber_kit)

