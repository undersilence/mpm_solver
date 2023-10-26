find_package(spdlog CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)

add_library(sim_dev STATIC utils/timer.cpp)

target_include_directories(sim_dev PUBLIC .)
target_link_libraries(sim_dev PUBLIC spdlog::spdlog_header_only taywee::args Eigen3::Eigen)
