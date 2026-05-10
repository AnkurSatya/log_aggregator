#pragma once
#include <functional>
#include <google/protobuf/message.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <utils/config.h>
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

  Messenger(std::shared_ptr<zmq::context_t> ctx, ZmqSocketConfig recv_config,
            MessageCallback recv_callback,
            std::shared_ptr<spdlog::logger> logger)
      : recv_config_{std::move(recv_config)}, ctx_{std::move(ctx)},
        recv_sock_{zmq::socket_t(*ctx_, recv_config.socket_type)},
        logger_{std::move(logger->clone("Messenger"))} {
    recv_sock_.bind(recv_config_.socket_addr);
    recv_sock_.set(zmq::sockopt::linger, 0);
    start_receiver(recv_callback);
  }

  void setup_sender(ZmqSocketConfig send_config) {
    send_config_ = send_config;
    send_sock_ =
        std::make_unique<zmq::socket_t>(*ctx_, send_config.socket_type);
    send_sock_->connect(send_config.socket_addr);
    send_sock_->set(zmq::sockopt::linger, 0);
  }

  void send(google::protobuf::Message &&msg) {
    // Calculates and allocates the exact size needed for the message.
    zmq::message_t zmq_msg(msg.ByteSizeLong());
    // Data is written directly into this allocated memory.
    msg.SerializeToArray(zmq_msg.data(), zmq_msg.size());
    // std::cout << "Sending message ---" << std::endl;
    // msg.PrintDebugString();

    auto send_flags = zmq::send_flags::none;
    if (send_config_) {
      send_flags = send_config_->send_flags;
    }
    send_sock_->send(std::move(zmq_msg), send_flags);
  }

  void start_receiver(MessageCallback msg_callback) {
    receiver_thread_ = std::jthread([this, msg_callback] {
      while (true) {
        try {
          zmq::message_t msg;
          if (this->recv_sock_.recv(msg)) {
            msg_callback(
                std::string(static_cast<char *>(msg.data()), msg.size()));
          }
        } catch (const zmq::error_t &e) {
          if (e.num() == ETERM) {
            std::cerr << e.what() << std::endl;
            break;
          } else
            std::cerr << "Error in messenger receiver: " << e.what()
                      << std::endl;
        }
      }
    });
  }

  ~Messenger() {
    recv_sock_.close();
    send_sock_->close();
  }

private:
  ZmqSocketConfig recv_config_;
  std::shared_ptr<zmq::context_t> ctx_;
  zmq::socket_t recv_sock_;
  std::jthread receiver_thread_;

  // Optional is used so that the config can be set lazily.
  std::optional<ZmqSocketConfig> send_config_;
  // Unique ptr is needed so that the actual socket can be created lazily(after
  // Messenger object initialisation), otherwise send_sock_ would have to be
  // initialised during the instantiation of Messenger.
  std::unique_ptr<zmq::socket_t> send_sock_;

  std::shared_ptr<spdlog::logger> logger_;
};
