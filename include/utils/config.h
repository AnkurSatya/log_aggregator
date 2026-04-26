#pragma once
#include <string>
#include <zmq.hpp>

struct ZmqSocketConfig {
  std::string sock_addr;
  zmq::socket_type socket_type;
  zmq::send_flags send_flags;
};
