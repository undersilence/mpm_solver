#pragma once
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace sim {

// define logger
class Logger {
public:
  Logger();
  static auto get() -> std::shared_ptr<spdlog::logger>;
};

} // namespace sim

#define LOG_FATAL(...) ::sim::Logger::get()->fatal(__VA_ARGS__)
#define LOG_ERROR(...) ::sim::Logger::get()->error(__VA_ARGS__)
#define LOG_WARN(...) ::sim::Logger::get()->warn(__VA_ARGS__)
#define LOG_INFO(...) ::sim::Logger::get()->info(__VA_ARGS__)
#define LOG_TRACE(...) ::sim::Logger::get()->trace(__VA_ARGS__)
