project( lintWrapperTesting )

add_executable( runTests simpleTest.cpp ../LinterItf.h
               ../LinterWrapperBase.h   ../LinterWrapperBase.cpp
               ../ClangTidyWrapper.h    ../ClangTidyWrapper.cpp
               ../ClazyWrapper.h        ../ClazyWrapper.cpp
               ../CallTotals.h ../LinterCombine.h ../LinterCombine.cpp)
target_link_libraries( runTests yaml-cpp )
target_compile_definitions( runTests PRIVATE CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/" )
target_compile_definitions( runTests PRIVATE CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}/" )
if( WIN32 )
    target_link_directories( runTests PRIVATE ${Boost_LIBRARY_DIR} )
elseif( UNIX )
    target_link_libraries( runTests yaml-cpp Boost::program_options Boost::thread )
endif()
