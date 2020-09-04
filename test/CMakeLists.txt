project(cpp-lint-combine_tests)

set(executableName cpp-lint-combine_tests)
file(GLOB LINT_COMBINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../src/*.cpp")
file(GLOB LINT_COMBINE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/../src/*.h"  )
list(REMOVE_ITEM LINT_COMBINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../src/main.cpp")
add_executable(${executableName} src/simpleTest.cpp ${LINT_COMBINE_SOURCES} ${LINT_COMBINE_HEADERS})

target_compile_definitions(${executableName} PRIVATE PATH_TO_VERSION_RESOURCE="${CMAKE_CURRENT_BINARY_DIR}/../version.rsrc")
target_compile_definitions(${executableName} PRIVATE CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")
target_compile_definitions(${executableName} PRIVATE CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}/")

if(WIN32)
    if(MSVC_VERSION AND MSVC_VERSION LESS 1924)
        add_custom_target("MSVC_VERSION_CHECK_TEST" ALL $<$<CONFIG:ReleaseChecked>:echo>
            "$<$<CONFIG:ReleaseChecked>: ERROR: ReleaseChecked configuration requires Visual Studio 2019+ v16.4+>")
    endif()
    target_link_directories(${executableName} PRIVATE ${Boost_LIBRARY_DIRS})
    set(ASAN_RT "${ASAN_LIBS_PATH}clang_rt.asan_dynamic")
    target_link_libraries(${executableName} yaml-cpp
        "$<$<CONFIG:ReleaseChecked>:${ASAN_RT}-i386.lib>"
        "$<$<CONFIG:ReleaseChecked>:${ASAN_RT}_runtime_thunk-i386.lib>")
    set(TARGET_DIR "$<TARGET_FILE_DIR:${executableName}>")
    set(ASAN_CMD_ARGS $<$<CONFIG:ReleaseChecked>:-E copy_if_different "${ASAN_RT}-i386.dll" "${TARGET_DIR}" >)
    add_custom_command(TARGET ${executableName} POST_BUILD COMMAND
        $<$<CONFIG:ReleaseChecked>:${CMAKE_COMMAND}> "${ASAN_CMD_ARGS}" COMMAND_EXPAND_LISTS)
elseif(UNIX)
    target_link_libraries(${executableName} yaml-cpp Boost::program_options Boost::thread)
endif()
add_test(${executableName} ${executableName})
