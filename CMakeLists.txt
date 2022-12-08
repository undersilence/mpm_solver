cmake_minimum_required(VERSION 3.0.0)
project(mpm_solver VERSION 0.1.0)
set (CMAKE_CXX_STANDARD 20)

add_executable(mpm_solver src/main.cpp src/ecs/ecs.cpp)
target_include_directories(mpm_solver PUBLIC "src")