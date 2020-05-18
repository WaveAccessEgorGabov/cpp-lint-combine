#define BOOST_TEST_MODULE lintWrapperTesting

#include "../LinterCombine.h"
#include "../LinterBase.h"
#include "../UsualFactory.h"

#include <boost/test/included/unit_test.hpp>
#include <boost/program_options.hpp>
#include <boost/predef.h>
#include <stdexcept>
#include <filesystem>
#include <iostream>

struct recoverFiles {
    ~recoverFiles() {
        std::filesystem::remove( CURRENT_SOURCE_DIR"yamlFiles/linterFile_1.yaml" );
        std::filesystem::copy_file( CURRENT_SOURCE_DIR"yamlFiles/linterFile_1_save.yaml",
                                    CURRENT_SOURCE_DIR"yamlFiles/linterFile_1.yaml" );
        std::filesystem::remove( CURRENT_SOURCE_DIR"yamlFiles/linterFile_2.yaml" );
        std::filesystem::copy_file( CURRENT_SOURCE_DIR"yamlFiles/linterFile_2_save.yaml",
                                    CURRENT_SOURCE_DIR"yamlFiles/linterFile_2.yaml" );
    }
};

class StreamCapture {
public:
    StreamCapture( std::ostream & stream ) : stream( & stream ) {
        old = stream.flush().rdbuf( buffer.rdbuf() );
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
    struct MockWrapper : LinterBase {
        MockWrapper( int argc, char ** argv, FactoryBase::Services & service ) : LinterBase( service ) {
            parseCommandLine( argc, argv );
        }

        void updateYamlAction( const YAML::Node & yamlNode ) const override {
        }

        void parseCommandLine( int argc, char ** argv ) override {
            name = argv[ 1 ];
            if( argc == 3 ) {
                yamlPath = argv[ 2 ];
            }
        }
    };

    class MocksFactory : public FactoryBase {
    public:
        MocksFactory( const MocksFactory & ) = delete;

        MocksFactory( MocksFactory && ) = delete;

        MocksFactory & operator=( MocksFactory const & ) = delete;

        MocksFactory & operator=( MocksFactory const && ) = delete;

        static MocksFactory & getInstance() {
            static MocksFactory mockFactory;
            return mockFactory;
        }

        std::shared_ptr < LinterItf >
        createLinter( int argc, char ** argv ) override {
            if( !strcmp( argv[ 0 ], "MockWrapper" ) ) {
                return std::make_shared < MockWrapper >( argc, argv, getService() );
            }
            return nullptr;
        }

    private:
        MocksFactory() {
        }
    };
}

BOOST_AUTO_TEST_SUITE( TestLinterCombineConstructor )

    BOOST_AUTO_TEST_CASE( emptyCommandLine ) {
        char * argv[] = { "" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 0 );
    }

    BOOST_AUTO_TEST_CASE( OneNotExistentLinter ) {
        char * argv[] = { "", "--sub-linter=NotExistentLinter" };
        int argc = sizeof( argv ) / sizeof( char * );
        BOOST_CHECK_THROW( LintCombine::LinterCombine( argc, argv ), std::logic_error );
    }

    BOOST_AUTO_TEST_CASE( FirstLinterNotExistentSecondExists ) {
        char * argv[] = { "", "--sub-linter=NotExistentLinter", "--sub-linter=clazy-standalone" };
        int argc = sizeof( argv ) / sizeof( char * );
        BOOST_CHECK_THROW( LintCombine::LinterCombine( argc, argv ), std::logic_error );
    }

    BOOST_AUTO_TEST_CASE( BothLintersNotExistent ) {
        char * argv[] = { "", "--sub-linter=NotExistentLinter_1", "--sub-linter=NotExistentLinter_2" };
        int argc = sizeof( argv ) / sizeof( char * );
        BOOST_CHECK_THROW( LintCombine::LinterCombine( argc, argv ), std::logic_error );
    }

    BOOST_AUTO_TEST_CASE( FirstLinterEmptySecondExists ) {
        char * argv[] = { "", "--sub-linter=", "--sub-linter=clazy-standalone" };
        int argc = sizeof( argv ) / sizeof( char * );
        BOOST_CHECK_THROW( LintCombine::LinterCombine( argc, argv ), std::logic_error );
    }

    BOOST_AUTO_TEST_CASE( BothLintersEmpty ) {
        char * argv[] = { "", "--sub-linter=", "--sub-linter=" };
        int argc = sizeof( argv ) / sizeof( char * );
        BOOST_CHECK_THROW( LintCombine::LinterCombine( argc, argv ), std::logic_error );
    }

    BOOST_AUTO_TEST_CASE( BothLintersExist ) {
        char * argv[] = { "", "--sub-linter=clang-tidy", "--sub-linter=clazy-standalone" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 2 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( BothLintersExistAndHasOptions ) {
        char * argv[] = { "", "--sub-linter=clang-tidy", "CTParam_1", "CTParam_2",
                          "--sub-linter=clazy-standalone", "CSParam_1", "CSParam_1" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 2 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions() == "CTParam_1 CTParam_2 " );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getOptions() == "CSParam_1 CSParam_1 " );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( BothLintersExistAndHasYamlPath ) {
        char * argv[] = { "", "--sub-linter=clang-tidy", "--export-fixes=CTFile.yaml",
                          "--sub-linter=clazy-standalone", "--export-fixes=CSFile.yaml" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 2 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath() == "CTFile.yaml" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getYamlPath() == "CSFile.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersExistAndFirstHasOptionsAndYamlPath ) {
        char * argv[] = { "", "--sub-linter=clang-tidy", "--export-fixes=CTFile.yaml", "CTParam_1", "CTParam_2",
                          "--sub-linter=clazy-standalone" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 2 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions() == "CTParam_1 CTParam_2 " );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath() == "CTFile.yaml" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( BothLintersExistAndHasOptionsAndYamlPath ) {
        char * argv[] = { "", "--sub-linter=clang-tidy", "--export-fixes=CTFile.yaml", "CTParam_1", "CTParam_2",
                          "--sub-linter=clazy-standalone", "--export-fixes=CSFile.yaml", "CSParam_1",
                          "CSParam_2", };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 2 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions() == "CTParam_1 CTParam_2 " );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath() == "CTFile.yaml" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getOptions() == "CSParam_1 CSParam_2 " );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getYamlPath() == "CSFile.yaml" );
    }

    BOOST_AUTO_TEST_CASE( LinterIsClangTidy ) {
        char * argv[] = { "", "--sub-linter=clang-tidy" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 1 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( LinterIsClazyStandalone ) {
        char * argv[] = { "", "--sub-linter=clazy-standalone" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 1 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( LinterIsClazyStandaloneAndOptionsExist ) {
        char * argv[] = { "", "--sub-linter=clazy-standalone", "lintParam_1", "lintParam_2" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 1 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions() == "lintParam_1 lintParam_2 " );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( LinterIsClazyStandaloneAndYamlPathExists ) {
        char * argv[] = { "", "--sub-linter=clazy-standalone", "--export-fixes=lintFile.yaml" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 1 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath() == "lintFile.yaml" );
    }

    BOOST_AUTO_TEST_CASE( LinterIsClazyStandaloneAndOptionsAndYamlPathExist ) {
        char * argv[] = { "", "--sub-linter=clazy-standalone", "--export-fixes=lintFile.yaml", "lintParam_1",
                          "lintParam_2" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        BOOST_CHECK( linterCombine.numLinters() == 1 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions() == "lintParam_1 lintParam_2 " );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getYamlPath() == "lintFile.yaml" );
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestCallAndWaitLinter )

    BOOST_AUTO_TEST_CASE( LinterTerminate ) {
        StreamCapture stdoutCapture( std::cout );
        std::string mockExecutableCommand_1;
        if( BOOST_OS_WINDOWS ) {
            mockExecutableCommand_1 = "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh";
        }
        if (BOOST_OS_LINUX) {
            mockExecutableCommand_1 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh";
        }
        char* argv[] = {
                "", "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_1.c_str())
        };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 3 );
        BOOST_CHECK(stdoutData.find("stdoutLinter_1") != std::string::npos);
    }

    BOOST_AUTO_TEST_CASE( LinterReturn1 ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        std::string mockExecutableCommand_1;
        if (BOOST_OS_WINDOWS) {
            mockExecutableCommand_1 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.bat";
        }
        if (BOOST_OS_LINUX) {
            mockExecutableCommand_1 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.sh";
        }
        char* argv[] = {
                "", "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_1.c_str())
        };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 3 );
        BOOST_CHECK(stdoutData.find("stdoutLinter_1") != std::string::npos);
        BOOST_CHECK(stderrData.find("stderrLinter_1") != std::string::npos);
    }

    BOOST_AUTO_TEST_CASE( FirstTerminateSecondReturn0WriteToStreamsWriteToFile ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        std::string mockExecutableCommand_1;
        std::string mockExecutableCommand_2;
        if (BOOST_OS_WINDOWS) {
            mockExecutableCommand_1 = "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh";
            mockExecutableCommand_2 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.bat";
        }
        if (BOOST_OS_LINUX) {
            mockExecutableCommand_1 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh";
            mockExecutableCommand_2 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.sh";
        }
        char * argv[] = {
                "", "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_1.c_str()),
                "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_2.c_str())
        };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 2 );
        BOOST_CHECK( stdoutData.find("stdoutLinter_1") != std::string::npos );
        BOOST_CHECK( stdoutData.find("stdoutLinter_2") != std::string::npos );
        BOOST_CHECK( stderrData.find("stderrLinter_2") != std::string::npos );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
        std::string fileData;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), fileData );
        BOOST_CHECK(fileData.find("this is linter_2") != std::string::npos);
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersTerminate ) {
        StreamCapture stdoutCapture( std::cout );
        std::string mockExecutableCommand_1;
        std::string mockExecutableCommand_2;
        if (BOOST_OS_WINDOWS) {
            mockExecutableCommand_1 = "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh";
            mockExecutableCommand_2 = "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_2.sh";
        }
        if (BOOST_OS_LINUX) {
            mockExecutableCommand_1 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh";
            mockExecutableCommand_2 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_2.sh";
        }
        char * argv[] = {
                "", "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_1.c_str()),
                "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_2.c_str())
        };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 3 );
        BOOST_CHECK( stdoutData.find("stdoutLinter_1") != std::string::npos );
        BOOST_CHECK( stdoutData.find("stdoutLinter_2") != std::string::npos );
    }

    BOOST_AUTO_TEST_CASE( FirstReturn1SecondReturn0WriteToStreamsWriteToFile ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        std::string mockExecutableCommand_1;
        std::string mockExecutableCommand_2;
        if (BOOST_OS_WINDOWS) {
            mockExecutableCommand_1 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.bat";
            mockExecutableCommand_2 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.bat";
        }
        if (BOOST_OS_LINUX) {
            mockExecutableCommand_1 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.sh";
            mockExecutableCommand_2 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.sh";
        }
        char* argv[] = {
                "", "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_1.c_str()),
                "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_2.c_str())
        };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 2 );
        BOOST_CHECK( stdoutData.find("stdoutLinter_1") != std::string::npos );
        BOOST_CHECK( stdoutData.find("stdoutLinter_2") != std::string::npos );
        BOOST_CHECK( stderrData.find("stderrLinter_1") != std::string::npos );
        BOOST_CHECK( stderrData.find("stderrLinter_2") != std::string::npos );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
        std::string fileData;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), fileData );
        BOOST_CHECK(fileData.find("this is linter_2") != std::string::npos);
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersReturn1 ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        std::string mockExecutableCommand_1;
        std::string mockExecutableCommand_2;
        if (BOOST_OS_WINDOWS) {
            mockExecutableCommand_1 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.bat";
            mockExecutableCommand_2 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_2.bat";
        }
        if (BOOST_OS_LINUX) {
            mockExecutableCommand_1 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.sh";
            mockExecutableCommand_2 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_2.sh";
        }
        char* argv[] = {
                "", "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_1.c_str()),
                "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_2.c_str())
        };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 3 );
        BOOST_CHECK( stdoutData.find("stdoutLinter_1") != std::string::npos );
        BOOST_CHECK( stdoutData.find("stdoutLinter_2") != std::string::npos );
        BOOST_CHECK( stderrData.find("stderrLinter_1") != std::string::npos );
        BOOST_CHECK( stderrData.find("stderrLinter_2") != std::string::npos );
    }

    BOOST_AUTO_TEST_CASE( LinterReturn0WriteToStreams ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        std::string mockExecutableCommand_1;
        if (BOOST_OS_WINDOWS) {
            mockExecutableCommand_1 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.bat";
        }
        if (BOOST_OS_LINUX) {
            mockExecutableCommand_1 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.sh";
        }
        char* argv[] = {
                "", "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_1.c_str())
        };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 0 );
        BOOST_CHECK(stdoutData.find("stdoutLinter_1") != std::string::npos);
        BOOST_CHECK(stderrData.find("stderrLinter_1") != std::string::npos);
    }

    BOOST_AUTO_TEST_CASE( LinterReturn0WriteToFile ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        std::string mockExecutableCommand_1;
        if (BOOST_OS_WINDOWS) {
            mockExecutableCommand_1 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_1.bat";
        }
        if (BOOST_OS_LINUX) {
            mockExecutableCommand_1 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_1.sh";
        }
        char* argv[] = {
                "", "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_1.c_str())
        };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 0 );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_1.yaml" ) );
        std::string fileData;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_1.yaml" ), fileData );
        BOOST_CHECK(fileData.find("this is linter_1") != std::string::npos);
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_1.yaml" );
    }

    BOOST_AUTO_TEST_CASE( LinterReturn0WriteToStreamsWriteToFile ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        std::string mockExecutableCommand_1;
        if (BOOST_OS_WINDOWS) {
            mockExecutableCommand_1 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_1.bat";
        }
        if (BOOST_OS_LINUX) {
            mockExecutableCommand_1 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_1.sh";
        }
        char* argv[] = {
                "", "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_1.c_str())
        };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 0 );
        BOOST_CHECK(stdoutData.find("stdoutLinter_1") != std::string::npos);
        BOOST_CHECK(stderrData.find("stderrLinter_1") != std::string::npos);
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_1.yaml" ) );
        std::string fileData;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_1.yaml" ), fileData );
        BOOST_CHECK(fileData.find("this is linter_1") != std::string::npos);
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_1.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersReturn0WriteToStreams ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        std::string mockExecutableCommand_1;
        std::string mockExecutableCommand_2;
        if (BOOST_OS_WINDOWS) {
            mockExecutableCommand_1 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.bat";
            mockExecutableCommand_2 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_2.bat";
        }
        if (BOOST_OS_LINUX) {
            mockExecutableCommand_1 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.sh";
            mockExecutableCommand_2 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_2.sh";
        }
        char* argv[] = {
                "", "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_1.c_str()),
                "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_2.c_str())
        };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 0 );
        BOOST_CHECK( stdoutData.find("stdoutLinter_1") != std::string::npos );
        BOOST_CHECK( stdoutData.find("stdoutLinter_2") != std::string::npos );
        BOOST_CHECK( stderrData.find("stderrLinter_1") != std::string::npos );
        BOOST_CHECK( stderrData.find("stderrLinter_2") != std::string::npos );
    }

    BOOST_AUTO_TEST_CASE( BothLintersReturn0WriteToFiles ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        std::string mockExecutableCommand_1;
        std::string mockExecutableCommand_2;
        if (BOOST_OS_WINDOWS) {
            mockExecutableCommand_1 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_1.bat";
            mockExecutableCommand_2 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_2.bat";
        }
        if (BOOST_OS_LINUX) {
            mockExecutableCommand_1 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_1.sh";
            mockExecutableCommand_2 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_2.sh";
        }
        char* argv[] = {
                "", "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_1.c_str()),
                "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_2.c_str())
        };

        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 0 );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_1.yaml" ) );
        std::string fileData_1;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_1.yaml" ), fileData_1 );
        BOOST_CHECK(fileData_1.find("this is linter_1") != std::string::npos);
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_1.yaml" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
        std::string fileData_2;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), fileData_2 );
        BOOST_CHECK(fileData_2.find("this is linter_2") != std::string::npos);
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    }

    BOOST_AUTO_TEST_CASE( FirstReturn0WriteToStreamsSecondReturn0WriteToFile ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        std::string mockExecutableCommand_1;
        std::string mockExecutableCommand_2;
        if (BOOST_OS_WINDOWS) {
            mockExecutableCommand_1 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.bat";
            mockExecutableCommand_2 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_2.bat";
        }
        if (BOOST_OS_LINUX) {
            mockExecutableCommand_1 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.sh";
            mockExecutableCommand_2 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_2.sh";
        }
        char* argv[] = {
                "", "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_1.c_str()),
                "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_2.c_str())
        };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 0 );
        BOOST_CHECK(stdoutData.find("stdoutLinter_1") != std::string::npos);
        BOOST_CHECK(stderrData.find("stderrLinter_1") != std::string::npos);
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
        std::string fileData;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), fileData );
        BOOST_CHECK(fileData.find("this is linter_2") != std::string::npos);
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersReturn0WriteToStreamsWriteToFiles ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        std::string mockExecutableCommand_1;
        std::string mockExecutableCommand_2;
        if (BOOST_OS_WINDOWS) {
            mockExecutableCommand_1 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_1.bat";
            mockExecutableCommand_2 = CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.bat";
        }
        if (BOOST_OS_LINUX) {
            mockExecutableCommand_1 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_1.sh";
            mockExecutableCommand_2 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.sh";
        }
        char* argv[] = {
                "", "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_1.c_str()),
                "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_2.c_str())
        };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 0 );
        BOOST_CHECK( stdoutData.find("stdoutLinter_1") != std::string::npos );
        BOOST_CHECK( stdoutData.find("stdoutLinter_2") != std::string::npos );
        BOOST_CHECK( stderrData.find("stderrLinter_1") != std::string::npos );
        BOOST_CHECK( stderrData.find("stderrLinter_2") != std::string::npos );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_1.yaml" ) );
        std::string fileData_1;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_1.yaml" ), fileData_1 );
        BOOST_CHECK(fileData_1.find("this is linter_1") != std::string::npos);
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_1.yaml" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
        std::string fileData_2;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), fileData_2 );
        BOOST_CHECK(fileData_2.find("this is linter_2") != std::string::npos);
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    }

    BOOST_AUTO_TEST_CASE( TestParallelWorking ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        std::string mockExecutableCommand_1;
        std::string mockExecutableCommand_2;
        if (BOOST_OS_WINDOWS) {
            mockExecutableCommand_1 = "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_1.sh";
            mockExecutableCommand_2 = "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_2.sh";
        }
        if (BOOST_OS_LINUX) {
            mockExecutableCommand_1 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_1.sh";
            mockExecutableCommand_2 = "sh " CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_2.sh";
        }
        char* argv[] = {
                "", "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_1.c_str()),
                "--sub-linter=MockWrapper",
                const_cast <char*> (mockExecutableCommand_2.c_str())
        };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 0 );
        BOOST_CHECK( stdoutData == "First_stdout_mes_1\nSecond_stdout_mes_1\nFirst_stdout_mes_2\n" );
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestUpdatedYaml )

    BOOST_AUTO_TEST_CASE( NotExistentYamlPath ) {
        char * argv[] = { "", "--sub-linter=MockWrapper", "defaultLinter", "NotExistentFile" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.success == 0 );
        BOOST_CHECK( callTotals.fail == 1 );
    }

    BOOST_AUTO_TEST_CASE( EmptyYamlPath ) {
        char * argv[] = { "", "--sub-linter=MockWrapper", "defaultLinter", "" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.success == 0 );
        BOOST_CHECK( callTotals.fail == 1 );
    }

    BOOST_AUTO_TEST_CASE( FirstHasEmptyYamlPathSecondHasPathToExistsYaml ) {
        char * argv[] = { "", "--sub-linter=MockWrapper", "defaultLinter", "",
                          "--sub-linter=MockWrapper", "defaultLinter",
                          CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.success == 1 );
        BOOST_CHECK( callTotals.fail == 1 );
        std::ifstream yamlFile_2( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" );
        std::ifstream yamlFile_2_save( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2_save.yaml" );
        std::istream_iterator < char > fileIter_2( yamlFile_2 ), end_2;
        std::istream_iterator < char > fileIter_2_save( yamlFile_2_save ), end_2_save;
        BOOST_CHECK_EQUAL_COLLECTIONS( fileIter_2, end_2, fileIter_2_save, end_2_save );
    }

    BOOST_AUTO_TEST_CASE( BothLintersHaveEmptyYamlPath ) {
        char * argv[] = { "", "--sub-linter=MockWrapper", "defaultLinter", "",
                          "--sub-linter=MockWrapper", "defaultLinter", "" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.success == 0 );
        BOOST_CHECK( callTotals.fail == 2 );
    }

    BOOST_AUTO_TEST_CASE( FirstHasNotExistentYamlPathSecondHasPathToExistsYaml ) {
        char * argv[] = { "", "--sub-linter=MockWrapper", "defaultLinter", "NotExistentFile",
                          "--sub-linter=MockWrapper", "defaultLinter",
                          CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.success == 1 );
        BOOST_CHECK( callTotals.fail == 1 );
        std::ifstream yamlFile_2( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" );
        std::ifstream yamlFile_2_save( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2_save.yaml" );
        std::istream_iterator < char > fileIter_2( yamlFile_2 ), end_2;
        std::istream_iterator < char > fileIter_2_save( yamlFile_2_save ), end_2_save;
        BOOST_CHECK_EQUAL_COLLECTIONS( fileIter_2, end_2, fileIter_2_save, end_2_save );
    }

    BOOST_AUTO_TEST_CASE( BothLintersHaveNotExistentYamlPath ) {
        char * argv[] = { "", "--sub-linter=MockWrapper", "defaultLinter", "NotExistentFile",
                          "--sub-linter=MockWrapper", "defaultLinter", "NotExistentFile" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.success == 0 );
        BOOST_CHECK( callTotals.fail == 2 );
    }

    BOOST_AUTO_TEST_CASE( YamlPathExists ) {
        char * argv[] = { "", "--sub-linter=MockWrapper", "defaultLinter",
                          CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.success == 1 );
        BOOST_CHECK( callTotals.fail == 0 );
        std::ifstream yamlFile_1( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml" );
        std::ifstream yamlFile_1_save( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1_save.yaml" );
        std::istream_iterator < char > fileIter_1( yamlFile_1 ), end_1;
        std::istream_iterator < char > fileIter_1_save( yamlFile_1_save ), end_1_save;
        BOOST_CHECK_EQUAL_COLLECTIONS( fileIter_1, end_1, fileIter_1_save, end_1_save );
    }

    BOOST_AUTO_TEST_CASE( BothLintersHaveExistYamlPath ) {
        char * argv[] = { "", "--sub-linter=MockWrapper", "defaultLinter",
                          CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml",
                          "--sub-linter=MockWrapper", "defaultLinter",
                          CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv, LintCombine::MocksFactory::getInstance() );
        LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.success == 2 );
        BOOST_CHECK( callTotals.fail == 0 );
        std::ifstream yamlFile_1( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml" );
        std::ifstream yamlFile_1_save( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1_save.yaml" );
        std::istream_iterator < char > fileIter_1( yamlFile_1 ), end_1;
        std::istream_iterator < char > fileIter_1_save( yamlFile_1_save ), end_1_save;
        BOOST_CHECK_EQUAL_COLLECTIONS( fileIter_1, end_1, fileIter_1_save, end_1_save );
        std::ifstream yamlFile_2( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" );
        std::ifstream yamlFile_2_save( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2_save.yaml" );
        std::istream_iterator < char > fileIter_2( yamlFile_2 ), end_2;
        std::istream_iterator < char > fileIter_2_save( yamlFile_2_save ), end_2_save;
        BOOST_CHECK_EQUAL_COLLECTIONS( fileIter_2, end_2, fileIter_2_save, end_2_save );
    }

    BOOST_FIXTURE_TEST_CASE( clangTidyTest, recoverFiles ) {
        char * argv[] = { "", "--sub-linter=clang-tidy",
                          "--export-fixes=" CURRENT_SOURCE_DIR "/yamlFiles/linterFile_1.yaml" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.success == 1 );
        BOOST_CHECK( callTotals.fail == 0 );

        YAML::Node yamlNode;
        yamlNode = YAML::LoadFile( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml" );
        for( auto it : yamlNode[ "Diagnostics" ] ) {
            std::ostringstream documentationLink;
            documentationLink << it[ "Documentation link" ];
            std::ostringstream ossToCompare;
            ossToCompare << "https://clang.llvm.org/extra/clang-tidy/checks/" << it[ "DiagnosticName" ]
                         << ".html";
            BOOST_CHECK( documentationLink.str() == ossToCompare.str() );
        }
    }

    BOOST_FIXTURE_TEST_CASE( clazyTest, recoverFiles ) {
        char * argv[] = { "", "--sub-linter=clazy-standalone",
                          "--export-fixes=" CURRENT_SOURCE_DIR "/yamlFiles/linterFile_2.yaml" };
        int argc = sizeof( argv ) / sizeof( char * );
        LintCombine::LinterCombine linterCombine( argc, argv );
        LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.success == 1 );
        BOOST_CHECK( callTotals.fail == 0 );

        YAML::Node yamlNode;
        yamlNode = YAML::LoadFile( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" );
        for( auto it : yamlNode[ "Diagnostics" ] ) {
            std::ostringstream documentationLink;
            documentationLink << it[ "Documentation link" ];
            std::ostringstream diagnosticName;
            diagnosticName << it[ "DiagnosticName" ];
            std::ostringstream ossToCompare;
            ossToCompare << "https://github.com/KDE/clazy/blob/master/docs/checks/README-";
            // substr() from 6 to size() for skipping "clazy-" in DiagnosticName
            ossToCompare << diagnosticName.str().substr( 6, diagnosticName.str().size() ) << ".md";
            BOOST_CHECK( documentationLink.str() == ossToCompare.str() );
        }
    }

BOOST_AUTO_TEST_SUITE_END()
