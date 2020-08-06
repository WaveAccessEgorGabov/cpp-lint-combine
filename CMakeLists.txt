# Ninja Multi-Config requires at least version 3.17
cmake_minimum_required(VERSION 3.17)
project(cpp-lint-combine)

if(CMAKE_CONFIGURATION_TYPES)
    if(NOT "ReleaseChecked" IN_LIST CMAKE_CONFIGURATION_TYPES)
        list(APPEND CMAKE_CONFIGURATION_TYPES ReleaseChecked)
    endif()
endif()

if(CMAKE_CONFIGURATION_TYPES)
    if("ReleaseChecked" IN_LIST CMAKE_CONFIGURATION_TYPES)
        # We have to parse path to the ASan libs from path to the compiler because 
        # there are not special variables (e.g. PATH) in CI with this path.
        # Example: 
        # Path to compiler is "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.26.28801/bin/Hostx86/x86/"
        # We take from this path "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/" 
        # and append path to Asan libs relatively taken path.
        string(REGEX REPLACE "(MSVC.*)" "Llvm/lib/clang/10.0.0/lib/windows/" ASAN_LIBS_PATH ${CMAKE_C_COMPILER})
        if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
            set(CMAKE_C_FLAGS_RELEASECHECKED          "${CMAKE_C_FLAGS_RELEASE}   /fsanitize=address /MD")
            set(CMAKE_CXX_FLAGS_RELEASECHECKED        "${CMAKE_CXX_FLAGS_RELEASE} /fsanitize=address /MD")
            set(ASAN_REQUIRED_LIB_1     /wholearchive:"${ASAN_LIBS_PATH}"clang_rt.asan_dynamic-i386.lib)
            set(ASAN_REQUIRED_LIB_2     /wholearchive:"${ASAN_LIBS_PATH}"clang_rt.asan_dynamic_runtime_thunk-i386.lib)
            set(CMAKE_EXE_LINKER_FLAGS_RELEASECHECKED "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${ASAN_REQUIRED_LIB_1} ${ASAN_REQUIRED_LIB_2}")                                 
        elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
            set(CMAKE_C_FLAGS_RELEASECHECKED          "${CMAKE_C_FLAGS_RELEASE}   -fsanitize=address,undefined,leak")
            set(CMAKE_CXX_FLAGS_RELEASECHECKED        "${CMAKE_CXX_FLAGS_RELEASE} -fsanitize=address,undefined,leak")
        elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
            set(CMAKE_C_FLAGS_RELEASECHECKED          "${CMAKE_C_FLAGS_RELEASE}   -fsanitize=address,undefined,leak")
            set(CMAKE_CXX_FLAGS_RELEASECHECKED        "${CMAKE_CXX_FLAGS_RELEASE} -fsanitize=address,undefined,leak")
        endif()
        set(CMAKE_EXE_LINKER_FLAGS_RELEASECHECKED     "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASECHECKED  "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
        set(CMAKE_STATIC_LINKER_FLAGS_RELEASECHECKED  "${CMAKE_STATIC_LINKER_FLAGS_RELEASE}")
        set(CMAKE_MODULE_LINKER_FLAGS_RELEASECHECKED  "${CMAKE_MODULE_LINKER_FLAGS_RELEASE}")
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)
if (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    add_definitions(-D_SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)  # for MSVC 2017
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 -D_WIN32_WINNT=0x0601")
endif ()

find_package(Boost 1.69 OPTIONAL_COMPONENTS program_options thread filesystem date_time REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

if(${Boost_VERSION_STRING} STREQUAL 1.72.0)
    message(FATAL_ERROR "Boost version 1.72.0 is not suitable due to an error"
                        "in boost::process, see https://github.com/boostorg/process/issues/116")
endif()

enable_testing()
add_subdirectory(test)

set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "disable parse tools")
add_subdirectory(yaml-cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp/include)

execute_process(WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMAND git describe --tags --long --abbrev=12 OUTPUT_VARIABLE cpp-lint-combineVersion)
string(REGEX REPLACE "-" "." cpp-lint-combineVersion ${cpp-lint-combineVersion})

set(PRODUCTNAME_STR_IMPL "LintCombine")
if (WIN32)
    STRING(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+" cpp-lint-combineVersion ${cpp-lint-combineVersion})
elseif (UNIX)
    STRING(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" cpp-lint-combineVersion ${cpp-lint-combineVersion})
endif ()

set(PRODUCTVERSION_STR_IMPL ${cpp-lint-combineVersion})
string(REGEX REPLACE "\\." "," cpp-lint-combineVersion ${cpp-lint-combineVersion})
set(FILEVERSION_STR_IMPL ${cpp-lint-combineVersion})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.rsrc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rsrc @ONLY)

set(executableName cpp-lint-combine)
file(GLOB LINT_COMBINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" )
file(GLOB LINT_COMBINE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h" )

add_executable(${executableName} ${LINT_COMBINE_SOURCES} ${LINT_COMBINE_HEADERS})
if (WIN32)
    target_link_directories(${executableName} PRIVATE ${Boost_LIBRARY_DIRS})
    target_sources(${executableName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/version.rc)
    target_link_libraries(${executableName} $<$<CONFIG:ReleaseChecked>:${ASAN_LIBS_PATH}clang_rt.asan_dynamic-i386.lib> yaml-cpp
                                            $<$<CONFIG:ReleaseChecked>:${ASAN_LIBS_PATH}clang_rt.asan_dynamic_runtime_thunk-i386.lib>)
    add_custom_command(TARGET ${executableName} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${ASAN_LIBS_PATH}/clang_rt.asan_dynamic-i386.dll 
            $<$<CONFIG:ReleaseChecked>:$<TARGET_FILE_DIR:${executableName}>> 
            $<$<NOT:$<CONFIG:ReleaseChecked>>:${ASAN_LIBS_PATH}>)
elseif (UNIX)
    target_link_libraries(${executableName} yaml-cpp Boost::program_options Boost::thread Boost::filesystem)
endif ()
target_compile_definitions(${executableName} PRIVATE CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}/")
target_compile_definitions(${executableName} PRIVATE PATH_TO_VERSION_RESOURCE="${CMAKE_CURRENT_BINARY_DIR}/version.rsrc")
