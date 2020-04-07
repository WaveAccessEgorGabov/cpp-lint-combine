cmake_minimum_required(VERSION 3.0)
project(clang_tidy_wrapper)

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS -pthread)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

add_subdirectory(yaml-cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp/include)

add_executable(clang_tidy_wrapper main.cpp)
target_compile_definitions(clang_tidy_wrapper PRIVATE CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(clang_tidy_wrapper yaml-cpp ${Boost_LIBRARIES})
