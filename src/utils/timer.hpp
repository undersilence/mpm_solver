//
// Created by 立 on 2022/12/21.
//
#pragma once
#include <chrono>
#include <thread>
#include "forward.hpp"
#include "utils/logger.hpp"

namespace sim {
class Timer {
public:
  Timer(const char* name);
  ~Timer();
  void stop();
private:
  const char* name;
  std::chrono::time_point<std::chrono::high_resolution_clock> start_point;
  bool is_stopped{false};
};
} // namespace sim

#define TIMER(name) ::sim::Timer timer##__LINE__(name)
#define FUNCTION_TIMER() TIMER(FUNCTION_SIG)
