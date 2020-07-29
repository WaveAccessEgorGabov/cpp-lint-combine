cmake_minimum_required(VERSION 3.14)
project(cpp-lint-combine)

set(CMAKE_CXX_STANDARD 17)
if (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    add_definitions(-D_SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)  # for MSVC 2017
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 -D_WIN32_WINNT=0x0601")
endif ()

set(Boost_USE_RELEASE_LIBS ON)  # only find release libs
find_package(Boost 1.69 OPTIONAL_COMPONENTS program_options thread filesystem date_time REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

if(${Boost_VERSION_STRING} STREQUAL 1.72.0)
    message( FATAL_ERROR "1.72.0 is an incorrect boost version bacause "
                         "it contains an error in the boost::process (https://github.com/boostorg/process/issues/116)" )
endif()
enable_testing()
add_subdirectory(test)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "disable parse tools")
add_subdirectory(yaml-cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp/include)

execute_process(WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMAND git describe --tags --long --abbrev=12 OUTPUT_VARIABLE LinterWrapperVersion)
string(REGEX REPLACE "-" "." LinterWrapperVersion ${LinterWrapperVersion})

set(PRODUCTNAME_STR_IMPL "LintCombine")
if (WIN32)
    STRING(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+" LinterWrapperVersion ${LinterWrapperVersion})
elseif (UNIX)
    STRING(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" LinterWrapperVersion ${LinterWrapperVersion})
endif ()

set(PRODUCTVERSION_STR_IMPL ${LinterWrapperVersion})
string(REGEX REPLACE "\\." "," LinterWrapperVersion ${LinterWrapperVersion})
set(FILEVERSION_STR_IMPL ${LinterWrapperVersion})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.rsrc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rsrc @ONLY)

set(executableName cpp-lint-combine)
file(GLOB LINT_COMBINE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" )
file(GLOB LINT_COMBINE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h" )

add_executable(${executableName} ${LINT_COMBINE_SRC} ${LINT_COMBINE_HEADERS} )
target_link_libraries(${executableName} yaml-cpp)
if (WIN32)
    target_link_directories(${executableName} PRIVATE ${Boost_LIBRARY_DIRS})
    target_sources(${executableName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/version.rc)
elseif (UNIX)
    target_link_libraries(${executableName} yaml-cpp Boost::program_options Boost::thread Boost::filesystem)
endif ()
target_compile_definitions(${executableName} PRIVATE CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}/")
target_compile_definitions(${executableName} PRIVATE PATH_TO_VERSION_RESOURCE="${CMAKE_CURRENT_BINARY_DIR}/version.rsrc")
