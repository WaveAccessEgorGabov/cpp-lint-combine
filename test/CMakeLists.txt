project(cpp-lint-combine_tests)

set(executableName cpp-lint-combine_tests)
file(GLOB LINT_COMBINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../src/*.cpp" )
file(GLOB LINT_COMBINE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/../src/*.h" )
list(REMOVE_ITEM LINT_COMBINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../src/main.cpp")
add_executable(${executableName} src/simpleTest.cpp ${LINT_COMBINE_SOURCES} ${LINT_COMBINE_HEADERS} )

target_compile_definitions(${executableName} PRIVATE PATH_TO_VERSION_RESOURCE="${CMAKE_CURRENT_BINARY_DIR}/../version.rsrc")
target_compile_definitions(${executableName} PRIVATE CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")
target_compile_definitions(${executableName} PRIVATE CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}/")

if (WIN32)
    target_link_directories(${executableName} PRIVATE ${Boost_LIBRARY_DIRS})
    target_link_libraries(${executableName} $<$<CONFIG:ReleaseChecked>:${ASAN_LIBS_PATH}clang_rt.asan_dynamic-i386.lib> yaml-cpp
                                            $<$<CONFIG:ReleaseChecked>:${ASAN_LIBS_PATH}clang_rt.asan_dynamic_runtime_thunk-i386.lib>)
    add_custom_command(TARGET ${executableName} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${ASAN_LIBS_PATH}/clang_rt.asan_dynamic-i386.dll 
            $<$<CONFIG:ReleaseChecked>:$<TARGET_FILE_DIR:${executableName}>> 
            $<$<NOT:$<CONFIG:ReleaseChecked>>:${ASAN_LIBS_PATH}>)
elseif (UNIX)
    target_link_libraries(${executableName} yaml-cpp Boost::program_options Boost::thread Boost::filesystem)
endif ()
add_test(${executableName} ${executableName})
