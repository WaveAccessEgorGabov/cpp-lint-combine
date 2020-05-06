// ToDo
#define BOOST_TEST_MODULE lintWrapperTesting

#include "../LinterCombine.h"
#include "../LinterWrapperBase.h"
#include "../FactoryBase.h"

#include <boost/test/included/unit_test.hpp>
#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <stdexcept>
#include <filesystem>

class StreamCapture {
public:
    explicit StreamCapture( std::ostream & stream ) : stream( & stream ) {
        stream.flush();
        old = stream.rdbuf( buffer.rdbuf() );
    }

    ~StreamCapture() {
        buffer.flush();
        stream->rdbuf( old );
    }

    std::string getBufferData() {
        return buffer.str();
    }

private:
    std::ostream * stream;
    std::ostringstream buffer;
    std::streambuf * old;
};

namespace LintCombine {
    struct MockWrapper : LinterWrapperBase {
        MockWrapper( int argc, char ** argv ) {
            parseCommandLine( argc, argv );
        }

        void updateYamlAction( const YAML::Node & yamlNode ) const override {
        }

        void parseCommandLine( int argc, char ** argv ) override {
            this->name = argv[ 0 ];
        }

    };

    class MockFactory : public FactoryBase {
    public:
        static MockFactory & getInstance();

        std::vector < std::shared_ptr < LinterItf>>
        createLinter( std::vector < std::pair < std::string, char ** >> lintersAndTheirOptions ) override {
            std::vector < std::shared_ptr < LinterItf>> linters;
            for( auto & it: lintersAndTheirOptions ) {
                int argc = sizeof( it.first ) / sizeof( char * );
                if( it.first == "MockWrapper" )
                    linters.emplace_back( std::make_shared < MockWrapper >( argc, it.second ) );
            }
        }
    };
}

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
                          "--sub-linter=", "clazy-standalone", "--export-fixes=", "CSFile.yaml", "CSParam_1",
                          "CSParam_2", };
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
        char * argv[] = { "--sub-linter=", "clang-tidy" };
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
        char * argv[] = { "--sub-linter=", "clazy-standalone" };
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
        char * argv[] = { "--sub-linter=", "clazy-standalone", "lintParam_1", "lintParam_2" };
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
        char * argv[] = { "--sub-linter=", "clazy-standalone", "--export-fixes=", "lintFile.yaml" };
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
                          "--export-fixes=", "lintFile.yaml", "lintParam_1", "lintParam_2" };
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

BOOST_AUTO_TEST_SUITE( TestCallAndWaitLinter )

    BOOST_AUTO_TEST_CASE( LinterTerminate ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        char * argv[] = { "--sub-linter=", "MockWrapper", "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 3 );
        BOOST_CHECK( stdoutCapture.getBufferData() == "stdoutLinter_1\n" );
        BOOST_CHECK( stderrCapture.getBufferData() == "stderrLinter_1\n" );
    }

    BOOST_AUTO_TEST_CASE( LinterReturn1 ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        char * argv[] = { "--sub-linter=", "MockWrapper", "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 3 );
        BOOST_CHECK( stdoutCapture.getBufferData() == "stdoutLinter_1\n" );
        BOOST_CHECK( stderrCapture.getBufferData() == "stderrLinter_1\n" );
    }

    BOOST_AUTO_TEST_CASE( FirstTerminateSecondReturn0WriteToStreamsWriteToFile ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        char * argv[] = { "--sub-linter=", "MockWrapper", "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh",
                          "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 2 );
        BOOST_CHECK( stdoutCapture.getBufferData() == "stdoutLinter_1\nstderrLinter_2\n" );
        BOOST_CHECK( stderrCapture.getBufferData() == "stderrLinter_1\nstderrLinter_2\n" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
        std::string str;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), str );
        BOOST_CHECK( str == "this is linter_2" );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersTerminate ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        char * argv[] = { "--sub-linter=", "MockWrapper", "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh",
                          "--sub-linter=", "MockWrapper", "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 3 );
        BOOST_CHECK( stdoutCapture.getBufferData() == "stdoutLinter_1\nstderrLinter_2\n" );
        BOOST_CHECK( stderrCapture.getBufferData() == "stderrLinter_1\nstderrLinter_2\n" );
    }

    BOOST_AUTO_TEST_CASE( FirstReturn1SecondReturn0WriteToStreamsWriteToFile ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        char * argv[] = { "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh",
                          "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 2 );
        BOOST_CHECK( stdoutCapture.getBufferData() == "stdoutLinter_1\nstderrLinter_2\n" );
        BOOST_CHECK( stderrCapture.getBufferData() == "stderrLinter_1\nstderrLinter_2\n" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
        std::string str;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), str );
        BOOST_CHECK( str == "this is linter_2" );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersReturn1 ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        char * argv[] = { "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh",
                          "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 3 );
        BOOST_CHECK( stdoutCapture.getBufferData() == "stdoutLinter_1\nstderrLinter_2\n" );
        BOOST_CHECK( stderrCapture.getBufferData() == "stderrLinter_1\nstderrLinter_2\n" );
    }

    BOOST_AUTO_TEST_CASE( LinterReturn0WriteToStreams ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        char * argv[] = { "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 0 );
        BOOST_CHECK( stdoutCapture.getBufferData() == "stdoutLinter_1\n" );
        BOOST_CHECK( stderrCapture.getBufferData() == "stderrLinter_1\n" );
    }

    BOOST_AUTO_TEST_CASE( LinterReturn0WriteToFile ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        char * argv[] = { "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 0 );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_1.yaml" ) );
        std::string str;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_1.yaml" ), str );
        BOOST_CHECK( str == "this is linter_1" );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_1.yaml" );
    }

    BOOST_AUTO_TEST_CASE( LinterReturn0WriteToStreamsWriteToFile ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        char * argv[] = { "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 0 );
        BOOST_CHECK( stdoutCapture.getBufferData() == "stdoutLinter_1\n" );
        BOOST_CHECK( stderrCapture.getBufferData() == "stderrLinter_1\n" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_1.yaml" ) );
        std::string str;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_1.yaml" ), str );
        BOOST_CHECK( str == "this is linter_1" );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_1.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersReturn0WriteToStreams ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        char * argv[] = { "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh",
                          "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 0 );
        BOOST_CHECK( stdoutCapture.getBufferData() == "stdoutLinter_1\nstderrLinter_2\n" );
        BOOST_CHECK( stderrCapture.getBufferData() == "stderrLinter_1\nstderrLinter_2\n" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersReturn0WriteToFiles ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        char * argv[] = { "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh",
                          "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 0 );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_1.yaml" ) );
        std::string str_1;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_1.yaml" ), str_1 );
        BOOST_CHECK( str_1 == "this is linter_1" );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_1.yaml" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
        std::string str_2;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), str_2 );
        BOOST_CHECK( str_2 == "this is linter_2" );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    }

    BOOST_AUTO_TEST_CASE( FirstReturn0WriteToStreamsSecondReturn0WriteToFile ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        char * argv[] = { "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh.sh",
                          "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh.sh" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 0 );
        BOOST_CHECK( stdoutCapture.getBufferData() == "stdoutLinter_1\n" );
        BOOST_CHECK( stderrCapture.getBufferData() == "stderrLinter_1\n" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
        std::string str;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), str );
        BOOST_CHECK( str == "this is linter_2" );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersReturn0WriteToStreamsWriteToFiles ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        char * argv[] = { "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh",
                          "--sub-linter=", "MockWrapper",
                          "sh" CURRENT_SOURCE_DIR "mockPrograms/.sh" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 0 );
        BOOST_CHECK( stdoutCapture.getBufferData() == "stdoutLinter_1\nstderrLinter_2\n" );
        BOOST_CHECK( stderrCapture.getBufferData() == "stderrLinter_1\nstderrLinter_2\n" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_1.yaml" ) );
        std::string str_1;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_1.yaml" ), str_1 );
        BOOST_CHECK( str_1 == "this is linter_1" );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_1.yaml" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
        std::string str_2;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), str_2 );
        BOOST_CHECK( str_2 == "this is linter_2" );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    }

BOOST_AUTO_TEST_SUITE_END()

