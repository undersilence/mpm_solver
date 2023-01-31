#include "utils/timer.hpp"

namespace sim {

Timer::Timer(const char* name) : name(name) {
  start_point = std::chrono::high_resolution_clock::now();
}

Timer::~Timer() {
  if (!is_stopped) {
    stop();
  }
}

void Timer::stop() {
  auto end_point = std::chrono::high_resolution_clock::now();
  int64_t start_count = std::chrono::time_point_cast<std::chrono::microseconds>(start_point)
                            .time_since_epoch()
                            .count();
  int64_t end_count =
      std::chrono::time_point_cast<std::chrono::microseconds>(end_point).time_since_epoch().count();
  // writes profiles here
  auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
  LOG_INFO("{} us elapsed for {} \t\tin thread {}.", end_count - start_count, name, tid);
  is_stopped = true;
}

} // namespace sim
