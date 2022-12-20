
add_executable(any_test ./test_copy.cpp)
add_executable(benchmark ./benchmark.cpp)
target_link_libraries(benchmark PUBLIC sim_ecs)
