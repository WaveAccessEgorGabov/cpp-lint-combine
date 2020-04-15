cmake_minimum_required(VERSION 3.0)
project(lintWrapper)

if(WIN32)
    if (NOT DEFINED ENV{LINTERS_DIR})
        message("environment variable <LINTERS_DIR> not defined")
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
	find_package(Boost COMPONENTS REQUIRED)
elseif(UNIX)
	find_package(Boost COMPONENTS program_options thread REQUIRED)
endif()
include_directories(${Boost_INCLUDE_DIR})

add_subdirectory(Tests)
add_subdirectory(yaml-cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp/include)

add_executable(lintWrapper main.cpp)
add_library(linkWrapperUtils lintWrapperUtils.cpp lintWrapperUtils.h)
target_compile_definitions(linkWrapperUtils PRIVATE CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
if(WIN32)
	target_link_libraries(linkWrapperUtils yaml-cpp)
	target_compile_definitions(linkWrapperUtils PRIVATE LINTERS_DIR="$ENV{LINTERS_DIR}")
	target_link_directories(linkWrapperUtils PRIVATE ${Boost_LIBRARY_DIR})
elseif(UNIX)
	target_link_libraries(linkWrapperUtils yaml-cpp Boost::program_options Boost::thread)
endif()
target_link_libraries(lintWrapper linkWrapperUtils)


