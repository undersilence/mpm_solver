#pragma once
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace sim::utils {

// define logger
class Logger {
public:
  Logger();
  static auto get() -> std::shared_ptr<spdlog::logger>;
};

} // namespace sim

#define LOG_FATAL(...) ::sim::utils::Logger::get()->fatal(__VA_ARGS__)
#define LOG_ERROR(...) ::sim::utils::Logger::get()->error(__VA_ARGS__)
#define LOG_WARN(...) ::sim::utils::Logger::get()->warn(__VA_ARGS__)
#define LOG_INFO(...) ::sim::utils::Logger::get()->info(__VA_ARGS__)
#define LOG_DEBUG(...) ::sim::utils::Logger::get()->trace(__VA_ARGS__)
