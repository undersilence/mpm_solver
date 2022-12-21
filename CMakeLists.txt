cmake_minimum_required(VERSION 3.0.0)
project(mpm_solver VERSION 0.1.0)
cmake_policy(SET CMP0054 NEW)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# https://github.com/izenecloud/cmake/blob/master/SetCompilerWarningAll.cmake
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # Use the highest warning level for Visual Studio.
    set(CMAKE_CXX_WARNING_LEVEL 4)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")

    # Disable C++ exceptions.
#    string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")
#    add_definitions(-D_HAS_EXCEPTIONS=0)

    # Disable RTTI.
    string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
else("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # Use -Wall for clang and gcc.
    if(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    endif(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")

    # Use -Wextra for clang and gcc.
    if(NOT CMAKE_CXX_FLAGS MATCHES "-Wextra")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
    endif(NOT CMAKE_CXX_FLAGS MATCHES "-Wextra")

    # Use -Werror for clang and gcc.
#    if(NOT CMAKE_CXX_FLAGS MATCHES "-Werror")
#        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
#    endif(NOT CMAKE_CXX_FLAGS MATCHES "-Werror")

    # Disable C++ exceptions.
#    string(REGEX REPLACE "-fexceptions" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

    # Disable RTTI.
    string(REGEX REPLACE "-frtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

include_directories("src")

find_package(spdlog CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
add_library(sim_ecs STATIC src/ecs/ecs.cpp src/ecs/vec.cpp)
#target_link_libraries(sim_ecs PUBLIC spdlog::spdlog_header_only)
#target_link_libraries(sim_ecs PUBLIC Eigen3::Eigen)

add_executable(mpm_solver src/main.cpp)
target_include_directories(mpm_solver PUBLIC "src")
target_link_libraries(mpm_solver PUBLIC sim_ecs)

add_subdirectory(./src/test)