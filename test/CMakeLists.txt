
# add_executable(any_test ./test_copy.cpp)
add_executable(benchmark ./benchmark.cpp)
target_link_libraries(benchmark PUBLIC sim_dev flecs::flecs_static) 
target_compile_definitions(benchmark PRIVATE TEST_NEO_ECS TEST_FLECS)
# add_executable(test_column ./test_column.cpp)

# target_link_libraries(test_column PUBLIC sim_dev)
