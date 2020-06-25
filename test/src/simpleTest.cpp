#define BOOST_TEST_MODULE lintWrapperTesting

#include "../../src/LinterCombine.h"
#include "../../src/LinterBase.h"
#include "../../src/LintCombineUtils.h"

#include <boost/test/included/unit_test.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <filesystem>

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
    explicit StreamCapture( std::ostream & stream ) : stream( & stream ) {
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
        MockWrapper( const stringVector & commandLine, FactoryBase::Services & service ) : LinterBase( service ) {
            name = commandLine[ 1 ];
            if( commandLine.size() >= 3 ) {
                yamlPath = commandLine[ 2 ];
            }
        }

        void updateYamlAction( const YAML::Node & ) const override {
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
        createLinter( const stringVector & commandLineSTL ) override {
            if( commandLineSTL[ 0 ] == "MockWrapper" ) {
                return std::make_shared < MockWrapper >( commandLineSTL, getServices() );
            }
            return nullptr;
        }

    private:
        MocksFactory() = default;
    };
}

BOOST_AUTO_TEST_SUITE( TestLinterCombineConstructor )

    BOOST_AUTO_TEST_CASE( emptyCommandLine ) {
        StreamCapture stderrCapture( std::cerr );
        LintCombine::stringVector commandLineSTL = {};
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.printTextIfRequested() == true );
        BOOST_CHECK( linterCombine.numLinters() == 0 );
        const std::string str = stderrCapture.getBufferData();
        BOOST_CHECK ( str.find ( "Product name" ) != std::string::npos );
        BOOST_CHECK ( str.find ( "Product version" ) != std::string::npos );
        BOOST_CHECK ( str.find ( "Program options" ) != std::string::npos );
        BOOST_CHECK ( str.find ( "Product name" ) != std::string::npos );
    }

    BOOST_AUTO_TEST_CASE( OneNotExistentLinter ) {
        const LintCombine::stringVector commandLineSTL = { "--sub-linter=NotExistentLinter" };
        BOOST_CHECK_THROW( LintCombine::LinterCombine { commandLineSTL }, std::logic_error );
    }

    BOOST_AUTO_TEST_CASE( FirstLinterNotExistentSecondExists ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=NotExistentLinter", "--sub-linter=clazy" };
        BOOST_CHECK_THROW( LintCombine::LinterCombine { commandLineSTL }, std::logic_error );
    }

    BOOST_AUTO_TEST_CASE( BothLintersNotExistent ) {
    const LintCombine::stringVector commandLineSTL = { "", "--sub-linter=NotExistentLinter_1",
                                                     "--sub-linter=NotExistentLinter_2" };
        BOOST_CHECK_THROW( LintCombine::LinterCombine { commandLineSTL }, std::logic_error );
    }

    BOOST_AUTO_TEST_CASE( FirstLinterEmptySecondExists ) {
        LintCombine::stringVector commandLineSTL = { "--sub-linter=", "--sub-linter=clazy" };
        BOOST_CHECK_THROW( LintCombine::LinterCombine { commandLineSTL }, std::logic_error );
    }

    BOOST_AUTO_TEST_CASE( BothLintersEmpty ) {
        LintCombine::stringVector commandLineSTL = { "--sub-linter=", "--sub-linter=" };
        BOOST_CHECK_THROW( LintCombine::LinterCombine { commandLineSTL }, std::logic_error );
    }

    BOOST_AUTO_TEST_CASE( BothLintersExist ) {
        LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy", "--sub-linter=clazy" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );

        BOOST_CHECK( linterCombine.numLinters() == 2 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( linterCombine.linterAt( 0 )->getYamlPath().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getOptions().empty() );
        BOOST_CHECK( linterCombine.linterAt( 1 )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( BothLintersExistAndHasOptions ) {
        LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy", "CTParam_1", "CTParam_2",
                                                     "--sub-linter=clazy", "CSParam_1", "CSParam_1" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.numLinters() == 2 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions() == "CTParam_1 CTParam_2 " );
        BOOST_CHECK( linterCombine.linterAt( 0 )->getYamlPath().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getOptions() == "CSParam_1 CSParam_1 " );
        BOOST_CHECK( linterCombine.linterAt( 1 )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( LinterIsClazyStandalone ) {
        LintCombine::stringVector commandLineSTL = { "--sub-linter=clazy" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.numLinters() == 1 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( linterCombine.linterAt( 0 )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( LinterIsClangTidy ) {
        LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.numLinters() == 1 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( linterCombine.linterAt( 0 )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( LinterIsClazyStandaloneAndOptionsExist ) {
        LintCombine::stringVector commandLineSTL = { "--sub-linter=clazy", "lintParam_1",
                                                     "lintParam_2" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.numLinters() == 1 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions() == "lintParam_1 lintParam_2 " );
        BOOST_CHECK( linterCombine.linterAt( 0 )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( LinterIsClazyStandaloneAndYamlPathExists ) {
        LintCombine::stringVector commandLineSTL = { "--sub-linter=clazy",
                                                     "--export-fixes="
                                                     CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.numLinters() == 1 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( linterCombine.linterAt( 0 )->getYamlPath() == CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" );
    }

    BOOST_AUTO_TEST_CASE( LinterIsClazyStandaloneAndOptionsAndYamlPathExist ) {
        LintCombine::stringVector commandLineSTL = { "--sub-linter=clazy",
                                                     "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml",
                                                     "lintParam_1", "lintParam_2" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.numLinters() == 1 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions() == "lintParam_1 lintParam_2 " );
        BOOST_CHECK( linterCombine.linterAt( 0 )->getYamlPath() == CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersExistFirstYamlPathNotSetSecondYamlPathIsEmpty ) {
        LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy",
                                                     "--sub-linter=clazy", "--export-fixes= " };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.numLinters() == 2 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( linterCombine.linterAt( 0 )->getYamlPath().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getOptions().empty() );
        BOOST_CHECK( linterCombine.linterAt( 1 )->getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( BothLintersExistAndFirstHasOptionsAndYamlPath ) {
        LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy", "--export-fixes=\\\\",
                                                     "--sub-linter=clazy",
                                                     "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.numLinters() == 2 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
        BOOST_CHECK( linterCombine.linterAt( 0 )->getYamlPath().empty() );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getOptions().empty() );
        BOOST_CHECK( linterCombine.linterAt( 1 )->getYamlPath() == CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersExistAndHasOptionsAndYamlPath ) {
        LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy",
                                                     "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml",
                                                     "CTParam_1", "CTParam_2",
                                                     "--sub-linter=clazy",
                                                     "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_2.yaml",
                                                     "CSParam_1",
                                                     "CSParam_2" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.numLinters() == 2 );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 0 ) )->getOptions() == "CTParam_1 CTParam_2 " );
        BOOST_CHECK( linterCombine.linterAt( 0 )->getYamlPath() == CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getName() == "clazy-standalone" );
        BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                             ( linterCombine.linterAt( 1 ) )->getOptions() == "CSParam_1 CSParam_2 " );
        BOOST_CHECK( linterCombine.linterAt( 1 )->getYamlPath() == CURRENT_SOURCE_DIR "yamlFiles/linterFile_2.yaml" );
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestCallAndWaitLinter )

    BOOST_AUTO_TEST_CASE( LinterTerminate ) {
        StreamCapture stdoutCapture( std::cout );
        LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLineSTL.emplace_back(
                    "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh" );
        }

        LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 3 );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
    }

    BOOST_AUTO_TEST_CASE( LinterReturn1 ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.bat" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.sh" );
        }

        LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 3 );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
        BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );
    }

    BOOST_AUTO_TEST_CASE( FirstTerminateSecondReturn0WriteToStreamsWriteToFile ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        LintCombine::stringVector commandLine = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLine.emplace_back(
                    "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh" );
            commandLine.emplace_back( "--sub-linter=MockWrapper" );
            commandLine.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.bat" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLine.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh" );
            commandLine.emplace_back( "--sub-linter=MockWrapper" );
            commandLine.emplace_back( "sh " CURRENT_SOURCE_DIR
                                         "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.sh" );
        }

        LintCombine::LinterCombine linterCombine( commandLine, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 2 );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_2" ) != std::string::npos );
        BOOST_CHECK( stderrData.find( "stderrLinter_2" ) != std::string::npos );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
        std::string fileData;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), fileData );
        BOOST_CHECK( fileData.find( "this is linter_2" ) != std::string::npos );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersTerminate ) {
        StreamCapture stdoutCapture( std::cout );
        LintCombine::stringVector commandLine = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLine.emplace_back(
                    "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh" );
            commandLine.emplace_back( "--sub-linter=MockWrapper" );
            commandLine.emplace_back(
                    "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_2.sh" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLine.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh" );
            commandLine.emplace_back( "--sub-linter=MockWrapper" );
            commandLine.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_2.sh" );
        }

        LintCombine::LinterCombine linterCombine( commandLine, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 3 );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_2" ) != std::string::npos );
    }

    BOOST_AUTO_TEST_CASE( FirstReturn1SecondReturn0WriteToStreamsWriteToFile ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        LintCombine::stringVector commandLine = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLine.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.bat" );
            commandLine.emplace_back( "--sub-linter=MockWrapper" );
            commandLine.emplace_back( CURRENT_SOURCE_DIR
                                         "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.bat" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLine.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.sh" );
            commandLine.emplace_back( "--sub-linter=MockWrapper" );
            commandLine.emplace_back( "sh " CURRENT_SOURCE_DIR
                                         "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.sh" );
        }

        LintCombine::LinterCombine linterCombine( commandLine, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 2 );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_2" ) != std::string::npos );
        BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );
        BOOST_CHECK( stderrData.find( "stderrLinter_2" ) != std::string::npos );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
        std::string fileData;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), fileData );
        BOOST_CHECK( fileData.find( "this is linter_2" ) != std::string::npos );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersReturn1 ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.bat" );
            commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
            commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_2.bat" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.sh" );
            commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_2.sh" );
        }

        LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 3 );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_2" ) != std::string::npos );
        BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );
        BOOST_CHECK( stderrData.find( "stderrLinter_2" ) != std::string::npos );
    }

    BOOST_AUTO_TEST_CASE( LinterReturn0WriteToStreams ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.bat" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.sh" );
        }

        LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 0 );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
        BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );
    }

    BOOST_AUTO_TEST_CASE( LinterReturn0WriteToFile ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_1.bat" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_1.sh" );
        }

        LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 0 );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_1.yaml" ) );
        std::string fileData;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_1.yaml" ), fileData );
        BOOST_CHECK( fileData.find( "this is linter_1" ) != std::string::npos );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_1.yaml" );
    }

    BOOST_AUTO_TEST_CASE( LinterReturn0WriteToStreamsWriteToFile ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_1.bat" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                         "mockPrograms/mockReturn0WriteToStreamsWriteToFile_1.sh" );
        }

        LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 0 );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
        BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_1.yaml" ) );
        std::string fileData;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_1.yaml" ), fileData );
        BOOST_CHECK( fileData.find( "this is linter_1" ) != std::string::npos );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_1.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersReturn0WriteToStreams ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.bat" );
            commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
            commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_2.bat" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.sh" );
            commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_2.sh" );
        }

        LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 0 );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_2" ) != std::string::npos );
        BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );
        BOOST_CHECK( stderrData.find( "stderrLinter_2" ) != std::string::npos );
    }

    BOOST_AUTO_TEST_CASE( BothLintersReturn0WriteToFiles ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_1.bat" );
            commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
            commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_2.bat" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_1.sh" );
            commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_2.sh" );
        }

        LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 0 );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_1.yaml" ) );
        std::string fileData_1;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_1.yaml" ), fileData_1 );
        BOOST_CHECK( fileData_1.find( "this is linter_1" ) != std::string::npos );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_1.yaml" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
        std::string fileData_2;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), fileData_2 );
        BOOST_CHECK( fileData_2.find( "this is linter_2" ) != std::string::npos );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    }

    BOOST_AUTO_TEST_CASE( FirstReturn0WriteToStreamsSecondReturn0WriteToFile ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.bat" );
            commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
            commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_2.bat" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.sh" );
            commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_2.sh" );
        }

        LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 0 );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
        BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
        std::string fileData;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), fileData );
        BOOST_CHECK( fileData.find( "this is linter_2" ) != std::string::npos );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    }

    BOOST_AUTO_TEST_CASE( BothLintersReturn0WriteToStreamsWriteToFiles ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
        LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLineSTL.emplace_back( CURRENT_SOURCE_DIR
                                         "mockPrograms/mockReturn0WriteToStreamsWriteToFile_1.bat" );
            commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
            commandLineSTL.emplace_back( CURRENT_SOURCE_DIR
                                         "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.bat" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                         "mockPrograms/mockReturn0WriteToStreamsWriteToFile_1.sh" );
            commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                         "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.sh" );
        }

        LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        std::string stderrData = stderrCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 0 );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
        BOOST_CHECK( stdoutData.find( "stdoutLinter_2" ) != std::string::npos );
        BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );
        BOOST_CHECK( stderrData.find( "stderrLinter_2" ) != std::string::npos );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_1.yaml" ) );
        std::string fileData_1;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_1.yaml" ), fileData_1 );
        BOOST_CHECK( fileData_1.find( "this is linter_1" ) != std::string::npos );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_1.yaml" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
        std::string fileData_2;
        getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), fileData_2 );
        BOOST_CHECK( fileData_2.find( "this is linter_2" ) != std::string::npos );
        std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    }

    BOOST_AUTO_TEST_CASE( TestParallelWorking ) {
        StreamCapture stdoutCapture( std::cout );
        LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_1.sh" );
            commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
            commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_2.sh" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_1.sh" );
            commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_2.sh" );
        }

        LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        int linterCombineReturnCode = linterCombine.waitLinter();
        std::string stdoutData = stdoutCapture.getBufferData();
        BOOST_CHECK( linterCombineReturnCode == 0 );
        BOOST_CHECK( stdoutData == "First_stdout_mes_1\nSecond_stdout_mes_1\nFirst_stdout_mes_2\n" );
    }

    BOOST_AUTO_TEST_CASE( OneLinterEndsEarlierThatCombine ) {
        std::ofstream( CURRENT_SOURCE_DIR "file_1.txt" );
        LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
                                         CURRENT_SOURCE_DIR "file_1.txt" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
                                         CURRENT_SOURCE_DIR "file_1.txt" );
        }

        LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 0 );
        BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR "file_1.txt" ) );
        std::filesystem::remove( CURRENT_SOURCE_DIR "file_1.txt" );
    }

    BOOST_AUTO_TEST_CASE( TwoLinterEndsEarlierThatCombine ) {
        std::ofstream( CURRENT_SOURCE_DIR "file_1.txt" );
        std::ofstream( CURRENT_SOURCE_DIR "file_2.txt" );
        LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper" };
        if constexpr ( BOOST_OS_WINDOWS ) {
            commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
                                         CURRENT_SOURCE_DIR"file_1.txt" );
            commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
            commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.3 "
                                         CURRENT_SOURCE_DIR"file_2.txt" );
        }
        if constexpr ( BOOST_OS_LINUX ) {
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
                                         CURRENT_SOURCE_DIR "file_1.txt" );
            commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
            commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.3 "
                                         CURRENT_SOURCE_DIR "file_2.txt" );
        }

        LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        linterCombine.callLinter();
        BOOST_CHECK( linterCombine.waitLinter() == 0 );
        BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR "file_1.txt" ) );
        std::filesystem::remove( CURRENT_SOURCE_DIR "file_1.txt" );
        BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR "file_2.txt" ) );
        std::filesystem::remove( CURRENT_SOURCE_DIR "file_2.txt" );
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestUpdatedYaml )

    BOOST_AUTO_TEST_CASE( NotExistentYamlPath ) {
        const LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper", "defaultLinter",
                                                     "NotExistentFile" };
        const LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        const LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.successNum == 0 );
        BOOST_CHECK( callTotals.failNum == 1 );
    }

    BOOST_AUTO_TEST_CASE( EmptyYamlPath ) {
        const LintCombine::stringVector commandLineSTL = { "", "--sub-linter=MockWrapper", "defaultLinter", "" };
        const LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        const LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.successNum == 0 );
        BOOST_CHECK( callTotals.failNum == 1 );
    }

    BOOST_AUTO_TEST_CASE( FirstHasEmptyYamlPathSecondHasPathToExistsYaml ) {
        const LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper", "defaultLinter", "",
                                                     "--sub-linter=MockWrapper", "defaultLinter",
                                                     CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" };
        const LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        const LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.successNum == 1 );
        BOOST_CHECK( callTotals.failNum == 1 );
        std::ifstream yamlFile_2( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" );
        std::ifstream yamlFile_2_save( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2_save.yaml" );
        std::istream_iterator < char > fileIter_2( yamlFile_2 ), end_2;
        std::istream_iterator < char > fileIter_2_save( yamlFile_2_save ), end_2_save;
        BOOST_CHECK_EQUAL_COLLECTIONS( fileIter_2, end_2, fileIter_2_save, end_2_save );
    }

    BOOST_AUTO_TEST_CASE( BothLintersHaveEmptyYamlPath ) {
        const LintCombine::stringVector commandLineSTL = { "", "--sub-linter=MockWrapper", "defaultLinter", "",
                                                     "--sub-linter=MockWrapper", "defaultLinter", "" };
        const LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        const LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.successNum == 0 );
        BOOST_CHECK( callTotals.failNum == 2 );
    }

    BOOST_AUTO_TEST_CASE( FirstHasNotExistentYamlPathSecondHasPathToExistsYaml ) {
        const LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper", "defaultLinter", "NotExistentFile",
                                                     "--sub-linter=MockWrapper", "defaultLinter",
                                                     CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" };
        const LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        const LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.successNum == 1 );
        BOOST_CHECK( callTotals.failNum == 1 );
        std::ifstream yamlFile_2( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" );
        std::ifstream yamlFile_2_save( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2_save.yaml" );
        std::istream_iterator < char > fileIter_2( yamlFile_2 ), end_2;
        std::istream_iterator < char > fileIter_2_save( yamlFile_2_save ), end_2_save;
        BOOST_CHECK_EQUAL_COLLECTIONS( fileIter_2, end_2, fileIter_2_save, end_2_save );
    }

    BOOST_AUTO_TEST_CASE( BothLintersHaveNotExistentYamlPath ) {
        const LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper", "defaultLinter", "NotExistentFile",
                                                     "--sub-linter=MockWrapper", "defaultLinter", "NotExistentFile" };
        const LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        const LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.successNum == 0 );
        BOOST_CHECK( callTotals.failNum == 2 );
    }

    BOOST_AUTO_TEST_CASE( YamlPathExists ) {
        const LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper", "defaultLinter",
                                                     CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml" };
        const LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        const LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.successNum == 1 );
        BOOST_CHECK( callTotals.failNum == 0 );
        std::ifstream yamlFile_1( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml" );
        std::ifstream yamlFile_1_save( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1_save.yaml" );
        std::istream_iterator < char > fileIter_1( yamlFile_1 ), end_1;
        std::istream_iterator < char > fileIter_1_save( yamlFile_1_save ), end_1_save;
        BOOST_CHECK_EQUAL_COLLECTIONS( fileIter_1, end_1, fileIter_1_save, end_1_save );
    }

    BOOST_AUTO_TEST_CASE( BothLintersHaveExistYamlPath ) {
        LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper", "defaultLinter",
                                                     CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml",
                                                     "--sub-linter=MockWrapper", "defaultLinter",
                                                     CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" };
        LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksFactory::getInstance() );
        LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.successNum == 2 );
        BOOST_CHECK( callTotals.failNum == 0 );
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
        LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy",
                                                     "--export-fixes="
                                                     CURRENT_SOURCE_DIR "/yamlFiles/linterFile_1.yaml" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.successNum == 1 );
        BOOST_CHECK( callTotals.failNum == 0 );

        std::ifstream yamlFile_1 ( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml" );
        std::ifstream yamlFile_1_save ( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1_result.yaml" );
        std::istream_iterator < char > fileIter_1 ( yamlFile_1 ), end_1;
        std::istream_iterator < char > fileIter_1_save ( yamlFile_1_save ), end_1_save;
        BOOST_CHECK_EQUAL_COLLECTIONS ( fileIter_1, end_1, fileIter_1_save, end_1_save );
    }

    BOOST_FIXTURE_TEST_CASE( clazyTest, recoverFiles ) {
        LintCombine::stringVector commandLine = { "--sub-linter=clazy",
                                                     "--export-fixes="
                                                     CURRENT_SOURCE_DIR "/yamlFiles/linterFile_2.yaml" };
        LintCombine::LinterCombine linterCombine( commandLine );
        LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        BOOST_CHECK( callTotals.successNum == 1 );
        BOOST_CHECK( callTotals.failNum == 0 );

        std::ifstream yamlFile_2 ( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" );
        std::ifstream yamlFile_2_save ( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2_result.yaml" );
        std::istream_iterator < char > fileIter_2 ( yamlFile_2 ), end_2;
        std::istream_iterator < char > fileIter_2_save ( yamlFile_2_save ), end_2_save;
        BOOST_CHECK_EQUAL_COLLECTIONS ( fileIter_2, end_2, fileIter_2_save, end_2_save );
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestMergeYaml )

    BOOST_AUTO_TEST_CASE( mergedYamlPathIsEmptyLintersYamlPathValid ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml=", "--sub-linter=clang-tidy",
                                                     "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
        BOOST_CHECK_THROW( LintCombine::LinterCombine { commandLineSTL }, boost::program_options::error );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlPathSetLintersYamlPathValid ) {
        LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy",
                                                     "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlPathNotValidLintersYamlPathValid ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml=\\\\", "--sub-linter=clang-tidy",
                                                     "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        std::cout << "\"" << linterCombine.getYamlPath() << "\"" << std::endl;
        BOOST_CHECK( linterCombine.getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlPathNotSetLintersYamlPathNotSet ) {
        LintCombine::stringVector commandLineSTL = { "", "--sub-linter=clang-tidy" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlValidLintersYamlPathNotSet ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                     CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                     "--sub-linter=clang-tidy" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.getYamlPath().empty() );
        BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" ) );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlValidLintersYamlPathIsEmpty ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                     CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                     "--sub-linter=clang-tidy", "--export-fixes=" };
        BOOST_CHECK_THROW( LintCombine::LinterCombine { commandLineSTL }, boost::program_options::error );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlValidLintersYamlPathNotValid ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                     CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                     "--sub-linter=clang-tidy", "--export-fixes=\\\\" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.getYamlPath().empty() );
        BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" ) );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlValidLintersYamlPathNotExists ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                     CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                     "--sub-linter=clang-tidy", "--export-fixes=NotExistentFile" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.getYamlPath().empty() );
        BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" ) );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlValidLintersYamlPathExists ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                     CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                     "--sub-linter=clang-tidy",
                                                     "--export-fixes="
                                                     CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.getYamlPath() == CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" ) );
        std::ifstream yamlFile( CURRENT_SOURCE_DIR"yamlFiles/linterFile_1.yaml" );
        std::ifstream combinedResult( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" );
        std::istream_iterator < char > yamlFileIt( yamlFile ), endYF;
        std::istream_iterator < char > combinedResultIt( combinedResult ), endCR;
        BOOST_CHECK_EQUAL_COLLECTIONS( yamlFileIt, endYF, combinedResultIt, endCR );
        combinedResult.close();
        std::filesystem::remove( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlValidFirstLintersYamlPathNotSetSecondValid ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                     CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                     "--sub-linter=clang-tidy",
                                                     "--sub-linter=clang-tidy",
                                                     "--export-fixes="
                                                     CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.getYamlPath() == CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" ) );
        std::ifstream yamlFile( CURRENT_SOURCE_DIR"yamlFiles/linterFile_1.yaml" );
        std::ifstream combinedResult( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" );
        std::istream_iterator < char > yamlFileIt( yamlFile ), endYF;
        std::istream_iterator < char > combinedResultIt( combinedResult ), endCR;
        BOOST_CHECK_EQUAL_COLLECTIONS( yamlFileIt, endYF, combinedResultIt, endCR );
        combinedResult.close();
        std::filesystem::remove( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlValidBothLintersYamlPathNotSet ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                     CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                     "--sub-linter=clang-tidy",
                                                     "--sub-linter=clang-tidy" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlValidFirstLintersYamlPathIsEmptySecondValid ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                     CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                     "--sub-linter=clang-tidy", "--export-fixes=",
                                                     "--sub-linter=clang-tidy",
                                                     "--export-fixes="
                                                     CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
        BOOST_CHECK_THROW( LintCombine::LinterCombine { commandLineSTL }, boost::program_options::error );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlValidBothLintersYamlPathIsEmpty ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                     CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                     "--sub-linter=clang-tidy", "--export-fixes=",
                                                     "--sub-linter=clang-tidy", "--export-fixes=" };
        BOOST_CHECK_THROW( LintCombine::LinterCombine { commandLineSTL }, boost::program_options::error );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlValidFirstLintersYamlPathNotValidSecondValid ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                     CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                     "--sub-linter=clang-tidy", "--export-fixes=\\\\",
                                                     "--sub-linter=clang-tidy",
                                                     "--export-fixes="
                                                     CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.getYamlPath() == CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" ) );
        std::ifstream yamlFile( CURRENT_SOURCE_DIR"yamlFiles/linterFile_1.yaml" );
        std::ifstream combinedResult( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" );
        std::istream_iterator < char > yamlFileIt( yamlFile ), endYF;
        std::istream_iterator < char > combinedResultIt( combinedResult ), endCR;
        BOOST_CHECK_EQUAL_COLLECTIONS( yamlFileIt, endYF, combinedResultIt, endCR );
        combinedResult.close();
        std::filesystem::remove( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlValidBothLintersYamlPathNotValid ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                     CURRENT_SOURCE_DIR "/yamlFiles/combinedResult.yaml",
                                                     "--sub-linter=clang-tidy", "--export-fixes=\\\\",
                                                     "--sub-linter=clang-tidy", "--export-fixes=\\\\" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlValidFirstLintersYamlPathNotExistsSecondValid ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                     CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                     "--sub-linter=clang-tidy", "--export-fixes=NotExistentFile",
                                                     "--sub-linter=clang-tidy",
                                                     "--export-fixes="
                                                     CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.getYamlPath() == CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" ) );
        std::ifstream yamlFile( CURRENT_SOURCE_DIR"yamlFiles/linterFile_1.yaml" );
        std::ifstream combinedResult( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" );
        std::istream_iterator < char > yamlFileIt( yamlFile ), endYF;
        std::istream_iterator < char > combinedResultIt( combinedResult ), endCR;
        BOOST_CHECK_EQUAL_COLLECTIONS( yamlFileIt, endYF, combinedResultIt, endCR );
        combinedResult.close();
        std::filesystem::remove( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlValidBothLintersYamlPathNotExists ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                     CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                     "--sub-linter=clang-tidy", "--export-fixes=NotExistentFile",
                                                     "--sub-linter=clang-tidy", "--export-fixes=NotExistentFile" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.getYamlPath().empty() );
    }

    BOOST_AUTO_TEST_CASE( mergedYamlValidBothLintersYamlPathValid ) {
        LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                     CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                     "--sub-linter=clang-tidy",
                                                     "--export-fixes="
                                                     CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml",
                                                     "--sub-linter=clang-tidy",
                                                     "--export-fixes="
                                                     CURRENT_SOURCE_DIR "yamlFiles/linterFile_2.yaml" };
        LintCombine::LinterCombine linterCombine( commandLineSTL );
        BOOST_CHECK( linterCombine.getYamlPath() == CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml" );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" ) );
        std::ifstream combinedResult_save( CURRENT_SOURCE_DIR"yamlFiles/combinedResult_save.yaml" );
        std::ifstream combinedResult( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" );
        std::istream_iterator < char > combinedResult_saveIt( combinedResult_save ), endCR_save;
        std::istream_iterator < char > combinedResultIt( combinedResult ), endCR;
        BOOST_CHECK_EQUAL_COLLECTIONS( combinedResult_saveIt, endCR_save, combinedResultIt, endCR );
        combinedResult.close();
        std::filesystem::remove( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" );
    }

BOOST_AUTO_TEST_SUITE_END()

//TODO: Figure out: why memory leak occurs in Debug
BOOST_AUTO_TEST_SUITE( TestPrepareCommandLine )

    void compareVectors( const LintCombine::stringVector & lhs, const LintCombine::stringVector & rhs ) {
        BOOST_REQUIRE ( lhs.size () == rhs.size () );
        for( size_t i = 0; i < lhs.size (); ++i ) {
            BOOST_CHECK ( lhs[ i ] == rhs[ i ] );
        }
    }

    BOOST_AUTO_TEST_CASE( TestVerbatimOptionExists ) {
        LintCombine::stringVector commandLine = { "--verbatim-commands", "param1",
                                                  "-p=pathToCompilationDataBase",
                                                  "--export-fixes=pathToResultYaml", "param2" };
        const LintCombine::stringVector result      = { "--verbatim-commands", "param1",
                                                  "-p=pathToCompilationDataBase",
                                                  "--export-fixes=pathToResultYaml", "param2" };

        LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
        compareVectors ( commandLine, result );
        BOOST_CHECK ( commandLinePreparer.getIsErrorWhilePrepareOccur () == false );
    }

    BOOST_AUTO_TEST_CASE ( TestVerbatimOptionNotExists ) {
        LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                                  "--export-fixes=pathToResultYaml" };
        const LintCombine::stringVector result = {
                "--result-yaml=pathToResultYaml",
                "--sub-linter=clang-tidy",
                "-p=pathToCompilationDataBase",
                "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
                "--sub-linter=clazy",
                "-p=pathToCompilationDataBase",
                "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };

        LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
        compareVectors ( commandLine, result );
        BOOST_CHECK ( commandLinePreparer.getIsErrorWhilePrepareOccur () == false );
    }

    BOOST_AUTO_TEST_CASE ( TestOptionForClangTidy ) {
        LintCombine::stringVector commandLine = { "-param_1",
                                                  "@param_2" };
        const LintCombine::stringVector result = {
                "--sub-linter=clang-tidy",
                "-param_1", "@param_2", "--sub-linter=clazy" };

        LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
        compareVectors ( commandLine, result );
        BOOST_CHECK ( commandLinePreparer.getIsErrorWhilePrepareOccur () == false );
    }

    BOOST_AUTO_TEST_CASE ( TestFilesToAnalizeAreSet ) {
        LintCombine::stringVector commandLine = { "file_1.cpp",
                                                  "file_2.cpp" };
        const LintCombine::stringVector result = {
                "--sub-linter=clang-tidy",
                "file_1.cpp", "file_2.cpp", "--sub-linter=clazy",
                "file_1.cpp", "file_2.cpp" };

        LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
        compareVectors ( commandLine, result );
        BOOST_CHECK ( commandLinePreparer.getIsErrorWhilePrepareOccur () == false );
    }

    BOOST_AUTO_TEST_CASE ( TestHeaderFilterSet ) {
        LintCombine::stringVector commandLine = { "-header-filter=file.cpp" };
        const LintCombine::stringVector result = {
                "--sub-linter=clang-tidy",
                "-header-filter=file.cpp", "--sub-linter=clazy",
                "-header-filter=file.cpp" };

        LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
        compareVectors ( commandLine, result );
        BOOST_CHECK ( commandLinePreparer.getIsErrorWhilePrepareOccur () == false );
    }

    BOOST_AUTO_TEST_CASE ( TestClazyChecksAreSet ) {
        LintCombine::stringVector commandLine = { "--clazy-checks=level0,level1" };
        const LintCombine::stringVector result = {
                "--sub-linter=clang-tidy",
                "--sub-linter=clazy",
                "--checks=level0,level1" };

        LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
        compareVectors ( commandLine, result );
        BOOST_CHECK ( commandLinePreparer.getIsErrorWhilePrepareOccur () == false );
    }

    BOOST_AUTO_TEST_CASE ( TestClangExtraArgs ) {
        LintCombine::stringVector commandLine = { "--clang-extra-args=arg_1 arg_2" };
        const LintCombine::stringVector result = {
                "--sub-linter=clang-tidy",
                "--sub-linter=clazy",
                "--extra-arg=arg_1", "--extra-arg=arg_2" };

        LintCombine::CommandLinePreparer commandLinePreparer ( commandLine, "ReSharper" );
        compareVectors ( commandLine, result );
        BOOST_CHECK ( commandLinePreparer.getIsErrorWhilePrepareOccur () == false );
    }

    BOOST_AUTO_TEST_CASE ( TestEmptyClazyChecks ) {
        LintCombine::stringVector commandLine = { "--clazy-checks=" };
        const LintCombine::stringVector result = {
                "--sub-linter=clang-tidy",
                "--sub-linter=clazy" };

        LintCombine::CommandLinePreparer commandLinePreparer ( commandLine, "ReSharper" );
        compareVectors ( commandLine, result );
        BOOST_CHECK ( commandLinePreparer.getIsErrorWhilePrepareOccur () == false );
    }

    BOOST_AUTO_TEST_CASE ( TestEmptyParams ) {
        LintCombine::stringVector commandLine = { "--clazy-checks= --clang-extra-args=" };
        const LintCombine::stringVector result = {
                "--sub-linter=clang-tidy",
                "--sub-linter=clazy" };

        LintCombine::CommandLinePreparer commandLinePreparer ( commandLine, "ReSharper" );
        compareVectors ( commandLine, result );
        BOOST_CHECK ( commandLinePreparer.getIsErrorWhilePrepareOccur () == false );
    }

    BOOST_AUTO_TEST_CASE ( TestEmptyCommandLine ) {
        LintCombine::stringVector commandLine = {};
        const LintCombine::stringVector result = {
                "--sub-linter=clang-tidy",
                "--sub-linter=clazy" };

        LintCombine::CommandLinePreparer commandLinePreparer ( commandLine, "ReSharper" );
        compareVectors ( commandLine, result );
        BOOST_CHECK ( commandLinePreparer.getIsErrorWhilePrepareOccur () == false );
    }

    BOOST_AUTO_TEST_CASE ( TestOneIncorrectLinterName ) {
        LintCombine::stringVector commandLine = { "--sub-linter=clang-clazy-tidy" };
        LintCombine::CommandLinePreparer commandLinePreparer ( commandLine, "ReSharper" );
        BOOST_CHECK ( commandLinePreparer.getIsErrorWhilePrepareOccur () == true );
    }

    BOOST_AUTO_TEST_CASE ( TestTwoIncorrectLintersNames ) {
        LintCombine::stringVector commandLine = { "--sub-linter=crazy", "--sub-linter=clang-tudy" };
        LintCombine::CommandLinePreparer commandLinePreparer ( commandLine, "ReSharper" );
        BOOST_CHECK ( commandLinePreparer.getIsErrorWhilePrepareOccur () == true );
    }

    BOOST_AUTO_TEST_CASE ( OnlyClangTidyExists ) {
        LintCombine::stringVector commandLine = { "--sub-linter=clang-tidy" };
        const LintCombine::stringVector result = { "--sub-linter=clang-tidy" };
        LintCombine::CommandLinePreparer commandLinePreparer ( commandLine, "ReSharper" );
        compareVectors ( commandLine, result );
        BOOST_CHECK ( commandLinePreparer.getIsErrorWhilePrepareOccur () == false );
    }

    BOOST_AUTO_TEST_CASE ( OnlyClazyExists ) {
        LintCombine::stringVector commandLine = { "--sub-linter=clazy" };
        const LintCombine::stringVector result = { "--sub-linter=clazy" };
        LintCombine::CommandLinePreparer commandLinePreparer ( commandLine, "ReSharper" );
        compareVectors ( commandLine, result );
        BOOST_CHECK ( commandLinePreparer.getIsErrorWhilePrepareOccur () == false );
    }

BOOST_AUTO_TEST_SUITE_END()
