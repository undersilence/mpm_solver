#include "utils/logger.hpp"

namespace sim::utils {
Logger::Logger() {
  get()->set_pattern("[%^%l%$]\t%v");
  get()->set_level(spdlog::level::level_enum::trace);
};
auto Logger::get() -> std::shared_ptr<spdlog::logger> {
  static auto logger = spdlog::stdout_color_mt("sim");
  return logger;
}
} // namespace sim