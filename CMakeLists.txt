cmake_minimum_required(VERSION 3.0)
project(lintWrapper)

if(WIN32)
    if (NOT DEFINED ENV{CLANGTIDY_PATH})
        message("environment variable <CLANGTIDY_PATH> not defined")
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)

find_package(Boost COMPONENTS program_options thread REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

add_subdirectory(yaml-cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp/include)

add_executable(lintWrapper main.cpp)
target_link_libraries(lintWrapper yaml-cpp Boost::program_options Boost::thread)
target_compile_definitions(lintWrapper PRIVATE CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
if(WIN32)
	target_compile_definitions(lintWrapper PRIVATE CLANGTIDY_PATH="$ENV{CLANGTIDY_PATH}")
    target_link_directories(lintWrapper ${Boost_LIBRARY_DIR})
endif()
