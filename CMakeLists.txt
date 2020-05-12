cmake_minimum_required(VERSION 3.0)
project(lintWrapper)

set(CMAKE_CXX_STANDARD 17)
if (CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif ()

# Boost version 1.72 has bug in boost::asio
# bug reference: https://github.com/boostorg/process/issues/116
if (WIN32)
    find_package(Boost 1.71 COMPONENTS REQUIRED)
elseif (UNIX)
    find_package(Boost 1.71 COMPONENTS program_options thread REQUIRED)
endif ()
include_directories(${Boost_INCLUDE_DIR})

add_subdirectory(Tests)
add_subdirectory(yaml-cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp/include)

execute_process(WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMAND git describe --tags --long --abbrev=12 OUTPUT_VARIABLE LinterWrapperVersion)
string(REGEX REPLACE "-" "." LinterWrapperVersion ${LinterWrapperVersion})

set(PRODUCTNAME_STR_IMPL "LinterWrapper")
if (WIN32)
    STRING(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+" LinterWrapperVersion ${LinterWrapperVersion})
elseif (UNIX)
    STRING(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" LinterWrapperVersion ${LinterWrapperVersion})
endif ()

set(PRODUCTVERSION_STR_IMPL ${LinterWrapperVersion})
string(REGEX REPLACE "\\." "," LinterWrapperVersion ${LinterWrapperVersion})
set(FILEVERSION_STR_IMPL ${LinterWrapperVersion})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.rsrc.in ${CMAKE_CURRENT_SOURCE_DIR}/version.rsrc @ONLY)

add_executable(runLinter main.cpp CallTotals.h
        LinterItf.h version.rsrc FactoryBase.h
        ClazyWrapper.h ClazyWrapper.cpp
        LinterCombine.h LinterCombine.cpp
        UsualFactory.h UsualFactory.cpp
        ClangTidyWrapper.h ClangTidyWrapper.cpp
        LinterBase.h LinterBase.cpp)
if (WIN32)
    target_link_libraries(runLinter yaml-cpp)
    target_link_directories(runLinter PRIVATE ${Boost_LIBRARY_DIR})
    target_sources(runLinter PRIVATE version.rc)
elseif (UNIX)
    target_link_libraries(runLinter yaml-cpp Boost::program_options Boost::thread)
endif ()
target_compile_definitions(runLinter PRIVATE CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")
