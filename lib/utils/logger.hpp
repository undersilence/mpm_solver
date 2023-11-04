#pragma once
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <spdlog/common.h>

namespace sim::utils {

// define logger
class Logger {
public:
  Logger();
  static inline std::shared_ptr<spdlog::logger> initialize() {
    auto logger = spdlog::stdout_color_mt("sim");
    logger->set_pattern("%^%=8l%$ %v");
    logger->set_level(spdlog::level::level_enum::debug);
    return logger;
  }

  static inline std::shared_ptr<spdlog::logger> get() {
    static auto logger = initialize();
    return logger;
  }
};

} // namespace sim::utils

#define LOG_FATAL(...) ::sim::utils::Logger::get()->fatal(__VA_ARGS__)
#define LOG_ERROR(...) ::sim::utils::Logger::get()->error(__VA_ARGS__)
#define LOG_WARN(...) ::sim::utils::Logger::get()->warn(__VA_ARGS__)
#define LOG_INFO(...) ::sim::utils::Logger::get()->info(__VA_ARGS__)
#define LOG_DEBUG(...) ::sim::utils::Logger::get()->debug(__VA_ARGS__)
