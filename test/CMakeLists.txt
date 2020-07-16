project(cpp-lint-combineTesting)

set(CMAKE_CXX_STANDARD 17)
if (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    add_definitions(-D_SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)  # for MSVC 2017
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 -D_WIN32_WINNT=0x0601")
endif ()

set(executableName cpp-lint-combineTesting)
add_executable(${executableName} src/simpleTest.cpp
        ../src/PrepareCmdLineItf.h       ../src/Diagnostic.h
        ../src/LinterItf.h               ../src/CallTotals.h
        ../src/LinterBase.h              ../src/LinterBase.cpp
        ../src/ClazyWrapper.h            ../src/ClazyWrapper.cpp
        ../src/UsualLinterFactory.h      ../src/UsualLinterFactory.cpp
        ../src/LinterCombine.h           ../src/LinterCombine.cpp
        ../src/LintCombineUtils.h        ../src/LintCombineUtils.cpp
        ../src/ClangTidyWrapper.h        ../src/ClangTidyWrapper.cpp
        ../src/PrepareCmdLineBase.h      ../src/PrepareCmdLineBase.cpp
        ../src/DiagnosticWorker.h        ../src/DiagnosticWorker.cpp
        ../src/PrepareCmdLineFactory.h   ../src/PrepareCmdLineFactory.cpp
        ../src/PrepareCmdLineVerbatim.h  ../src/PrepareCmdLineVerbatim.cpp
        ../src/PrepareCmdLineReSharper.h ../src/PrepareCmdLineReSharper.cpp )

target_compile_definitions(${executableName} PRIVATE PATH_TO_VERSION_RESOURCE="${CMAKE_CURRENT_BINARY_DIR}/../version.rsrc")
target_compile_definitions(${executableName} PRIVATE CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")
target_compile_definitions(${executableName} PRIVATE CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}/")
target_link_libraries(${executableName} yaml-cpp)
