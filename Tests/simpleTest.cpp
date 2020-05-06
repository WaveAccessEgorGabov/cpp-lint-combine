// ToDo use abstract factory
#define BOOST_TEST_MODULE lintWrapperTesting

#include "../LinterCombine.h"
#include "../LinterWrapperBase.h"

#include <boost/test/included/unit_test.hpp>
#include <boost/program_options.hpp>
#include <stdexcept>

BOOST_AUTO_TEST_SUITE( TestLinterCombineConstructor )

    BOOST_AUTO_TEST_CASE( emptyCommandLine ) {
        char * argv[] = { nullptr };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 0 );
    }

    BOOST_AUTO_TEST_CASE( OneNotExistentLinter ) {
        char * argv[] = { "--sub-linter=", "NotExistentLinter" };
        int argc = sizeof( argv ) / sizeof( char * );
        BOOST_CHECK_THROW( LintCombine::LinterCombine( argc, argv ), std::logic_error );
    }

    BOOST_AUTO_TEST_CASE( FirstLinterNotExistentSecondExists ) {
        char * argv[] = { "--sub-linter=", "NotExistentLinter", "--sub-linter=", "clazy-standalone" };
        int argc = sizeof( argv ) / sizeof( char * );
        BOOST_CHECK_THROW( LintCombine::LinterCombine( argc, argv ), std::logic_error );
    }

    BOOST_AUTO_TEST_CASE( BothLintersNotExistent ) {
        char * argv[] = { "--sub-linter=", "NotExistentLinter_1", "--sub-linter=", "NotExistentLinter_2" };
        int argc = sizeof( argv ) / sizeof( char * );
        BOOST_CHECK_THROW( LintCombine::LinterCombine( argc, argv ), std::logic_error );
    }

    BOOST_AUTO_TEST_CASE( FirstLinterEmptySecondExists ) {
        char * argv[] = { "--sub-linter=", "", "--sub-linter=", "clazy-standalone" };
        int argc = sizeof( argv ) / sizeof( char * );
        BOOST_CHECK_THROW( LintCombine::LinterCombine( argc, argv ), boost::program_options::error );
    }

    BOOST_AUTO_TEST_CASE( BothLintersEmpty ) {
        char * argv[] = { "--sub-linter=", "", "--sub-linter=", "" };
        int argc = sizeof( argv ) / sizeof( char * );
        BOOST_CHECK_THROW( LintCombine::LinterCombine( argc, argv ), boost::program_options::error );
    }

    BOOST_AUTO_TEST_CASE( BothLintersExist ) {
        char * argv[] = { "--sub-linter=", "clang-tidy", "--sub-linter=", "clazy-standalone" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 2 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 1 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 1 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 1 ) )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( BothLintersExistAndHasOptions ) {
        char * argv[] = { "--sub-linter=", "clang-tidy", "CTParam_1", "CTParam_2",
                          "--sub-linter=", "clazy-standalone", "CSParam_1", "CSParam_1" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 2 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions() == "CTParam_1 CTParam_2" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 1 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 1 ) )->getOptions() == "CSParam_1 CSParam_1" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 1 ) )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( BothLintersExistAndHasYamlPath ) {
        char * argv[] = { "--sub-linter=", "clang-tidy", "--export-fixes=", "CTFile.yaml",
                          "--sub-linter=", "clazy-standalone", "--export-fixes=", "CSFile.yaml" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 2 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath() == "CTFile.yaml" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 1 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 1 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 1 ) )->getYamlPath() == "CSFile.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersExistAndFirstHasOptionsAndYamlPath ) {
        char * argv[] = { "--sub-linter=", "clang-tidy", "--export-fixes=", "CTFile.yaml", "CTParam_1", "CTParam_2",
                          "--sub-linter=", "clazy-standalone" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 2 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions() == "CTParam_1 CTParam_2" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath() == "CTFile.yaml" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 1 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 1 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 1 ) )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( BothLintersExistAndHasOptionsAndYamlPath ) {
        char * argv[] = { "--sub-linter=", "clang-tidy", "--export-fixes=", "CTFile.yaml", "CTParam_1", "CTParam_2",
                          "--sub-linter=", "clazy-standalone", "--export-fixes=", "CSFile.yaml", "CSParam_1", "CSParam_2", };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 2 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions() == "CTParam_1 CTParam_2" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath() == "CTFile.yaml" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 1 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 1 ) )->getOptions() == "CSParam_1 CSParam_2" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 1 ) )->getYamlPath() == "CSFile.yaml" );
    }

    BOOST_AUTO_TEST_CASE( LinterIsClangTidy ) {
        char * argv[] = { "--sub-linter=", "clang-tidy"};
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 1 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( LinterIsClazyStandalone ) {
        char * argv[] = { "--sub-linter=", "clazy-standalone"};
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 1 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( LinterIsClazyStandaloneAndOptionsExist ) {
        char * argv[] = { "--sub-linter=", "clazy-standalone", "lintParam_1", "lintParam_2"};
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 1 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions() == "lintParam_1 lintParam_2" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( LinterIsClazyStandaloneAndYamlPathExists ) {
        char * argv[] = { "--sub-linter=", "clazy-standalone", "--export-fixes=", "lintFile.yaml"};
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 1 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath() == "lintFile.yaml" );
    }

    BOOST_AUTO_TEST_CASE( LinterIsClazyStandaloneAndOptionsAndYamlPathExist ) {
        char * argv[] = { "--sub-linter=", "clazy-standalone",
                          "--export-fixes=", "lintFile.yaml", "lintParam_1", "lintParam_2"};
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 1 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions() == "lintParam_1 lintParam_2" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterWrapperBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath() == "lintFile.yaml" );
    }

BOOST_AUTO_TEST_SUITE_END()
