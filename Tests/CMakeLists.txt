project(lintWrapperTesting)

add_executable(runTests src/simpleTest.cpp
        ../src/LinterItf.h        ../src/CallTotals.h
        ../src/LinterBase.h       ../src/LinterBase.cpp
        ../src/ClazyWrapper.h     ../src/ClazyWrapper.cpp
        ../src/UsualFactory.h     ../src/UsualFactory.cpp
        ../src/LinterCombine.h    ../src/LinterCombine.cpp
        ../src/LintCombineUtils.h ../src/LintCombineUtils.cpp
        ../src/ClangTidyWrapper.h ../src/ClangTidyWrapper.cpp )
target_link_libraries(runTests yaml-cpp)
target_compile_definitions(runTests PRIVATE CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")
target_compile_definitions(runTests PRIVATE CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}/")
if (WIN32)
    target_link_directories(runTests PRIVATE ${Boost_LIBRARY_DIR})
elseif (UNIX)
    target_link_libraries(runTests yaml-cpp Boost::program_options Boost::thread Boost::filesystem)
endif ()
