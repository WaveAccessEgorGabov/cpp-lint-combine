project(lintWrapperTesting)

add_executable(runTests simpleTest.cpp  ../LinterWrapperItf.h
               ../LinterWrapperBase.h   ../LinterWrapperBase.cpp
               ../ClangTidyWrapper.h    ../ClangTidyWrapper.cpp
               ../ClazyWrapper.h        ../ClazyWrapper.cpp
               ../LinterWrapperUtils.h  ../LinterWrapperUtils.cpp)
target_link_libraries(runTests yaml-cpp)
target_compile_definitions(runTests PRIVATE CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")
target_compile_definitions(runTests PRIVATE CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}/")
if (WIN32)
    target_link_directories(runTests PRIVATE ${Boost_LIBRARY_DIR})
endif ()
add_executable(mockReturn0 mockPrograms/mockReturn0Source.cpp)
add_executable(mockReturn1 mockPrograms/mockReturn1Source.cpp)
add_executable(mockReturn2 mockPrograms/mockReturn2Source.cpp)
