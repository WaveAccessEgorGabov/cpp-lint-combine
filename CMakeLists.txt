cmake_minimum_required(VERSION 3.0)
project(lintWrapper)

set(CMAKE_CXX_STANDARD 17)

if (WIN32)
    find_package(Boost COMPONENTS REQUIRED)
elseif (UNIX)
    find_package(Boost COMPONENTS program_options thread REQUIRED)
endif ()
include_directories(${Boost_INCLUDE_DIR})

add_subdirectory(Tests)
add_subdirectory(yaml-cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp/include)

add_executable(runLinter main.cpp)
add_library(LinterWrapper LinterWrapperItf.h
        	LinterWrapperBase.h  LinterWrapperBase.cpp
        	ClangTidyWrapper.h   ClangTidyWrapper.cpp
        	ClazyWrapper.h       ClazyWrapper.cpp
        	LinterWrapperUtils.h LinterWrapperUtils.cpp)
if (WIN32)
    target_link_libraries(LinterWrapper yaml-cpp)
    target_link_directories(LinterWrapper PRIVATE ${Boost_LIBRARY_DIR})
elseif (UNIX)
    target_link_libraries(LinterWrapper yaml-cpp Boost::program_options Boost::thread)
endif ()
target_link_libraries(runLinter LinterWrapper)
target_compile_definitions(LinterWrapper PRIVATE CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")


