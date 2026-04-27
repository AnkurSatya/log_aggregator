#pragma once
#include <functional>
#include <google/protobuf/message.h>
#include <thread>
#include <zmq.hpp>

using MessageCallback = std::function<void(const std::string &)>;

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

  void send(const google::protobuf::Message &msg) {
    // Calculates and allocates the exact size needed for the message.
    zmq::message_t zmq_msg(msg.ByteSizeLong());
    // Data is written directly into this allocated memory.
    msg.SerializeToArray(zmq_msg.data(), zmq_msg.size());
    sock_.send(std::move(zmq_msg), send_flags_);
  }

  void start_receiver(MessageCallback msg_callback) {
    std::jthread([this, msg_callback]() {
      while (true) {
        try {
          zmq::message_t msg;
          if (sock_.recv(msg)) {
            msg_callback(
                std::string(static_cast<char *>(msg.data()), msg.size()));
          }
        } catch (const zmq::error_t &e) {
          if (e.num() == ETERM)
            break;
          else
            std::cerr << "Error in messenger receiver: " << e.what()
                      << std::endl;
        }
      }
    });
  }

private:
  std::shared_ptr<zmq::context_t> ctx_;
  std::string sock_addr_;
  zmq::send_flags send_flags_{zmq::send_flags::dontwait};
  zmq::socket_t sock_;
};
