#pragma once
#include <string>
#include <zmq.hpp>

struct ZmqSocketConfig {
  std::string socket_addr;
  zmq::socket_type socket_type;
  zmq::send_flags send_flags;
  zmq::recv_flags recv_flags;
  // ToDo:
  // 1. Create a variant for ZmqSockopt which should be able to take int,
  // string, bool etc.
  // 2. Then create a vector of this variant to store different sock options.
};
