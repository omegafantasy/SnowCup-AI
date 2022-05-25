#ifndef STREAM_HELPER_H
#define STREAM_HELPER_H

#include <iostream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace thuai {
  void i32_to_bytes(int32_t, bool, char*);
  int32_t bytes_to_i32(bool, char*);
  void write_to_judger(json j);
}

#endif