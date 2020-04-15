project(lintWrapperTesting)

add_executable(runTests simpleTest.cpp)
target_link_libraries(runTests LinterWrapper)
target_compile_definitions(runTests PRIVATE CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(runTests PRIVATE CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")
add_executable(mockReturn0 mockPrograms/mockReturn0Source.cpp)
add_executable(mockReturn1 mockPrograms/mockReturn1Source.cpp)
add_executable(mockReturn2 mockPrograms/mockReturn2Source.cpp)
