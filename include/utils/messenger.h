#pragma once
#include <zmq.hpp>

class Messenger {
public:
  Messenger(Messenger &&) = delete; // Moving is not allowed
  Messenger &
  operator=(Messenger &&) = delete; // Reassignment moving is not allowed

  Messenger(const Messenger &) = delete; // Copying is not allowed
  Messenger &
  operator=(Messenger &) = delete; // Copying reassignment is not allowed

  Messenger(std::shared_ptr<zmq::context_t> ctx, std::string sock_addr,
            zmq::socket_type socket_type, zmq::send_flags send_flags,
            bool is_binder)
      : ctx_{std::move(ctx)}, sock_addr_{std::move(sock_addr)},
        send_flags_(send_flags), sock_{zmq::socket_t(*ctx_, socket_type)} {

    // To prevent the the thread that closes the ZMQ socket created here from
    // hanging. In its absence, ZMQ would block the thread from closing until
    // all the messages have been removed from the queue or a timeout has
    // reached. Setting linger to 0 gives a snappy exit.
    sock_.set(zmq::sockopt::linger, 0);
    if (is_binder)
      sock_.bind(sock_addr_);
    else
      sock_.connect(sock_addr);
  }

  void send(std::string_view message) {
    sock_.send(zmq::message_t{std::move(message)}, send_flags_);
  }

  void recv() {}

private:
  std::shared_ptr<zmq::context_t> ctx_;
  std::string sock_addr_;
  zmq::send_flags send_flags_{zmq::send_flags::dontwait};
  zmq::socket_t sock_;
};
