cmake_minimum_required(VERSION 3.0)
project(clangTidyWrapper)

if(WIN32)
    if (NOT DEFINED ENV{CLANGTIDY_PATH})
        message("environment variable <CLANGTIDY_PATH> not defined")
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)

if(UNIX)
    SET(CMAKE_CXX_FLAGS -pthread)
endif()

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

add_subdirectory(yaml-cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp/include)

add_executable(clangTidyWrapper main.cpp)
target_link_libraries(clangTidyWrapper yaml-cpp ${Boost_LIBRARIES})
target_compile_definitions(clangTidyWrapper PRIVATE CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
if(WIN32)
	target_compile_definitions(clangTidyWrapper PRIVATE CLANGTIDY_PATH="$ENV{CLANGTIDY_PATH}")
    target_link_directories(clangTidyWrapper ${Boost_LIBRARY_DIR})
endif()
