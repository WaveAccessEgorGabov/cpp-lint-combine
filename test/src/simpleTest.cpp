#define BOOST_TEST_MODULE lintWrapperTesting

#include "../../src/LinterCombine.h"
#include "../../src/LinterBase.h"
#include "../../src/PrepareCmdLineFactory.h"
#include "../../src/PrepareCmdLineItf.h"

#include <boost/test/included/unit_test.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <filesystem>

struct recoverYamlFiles {
    ~recoverYamlFiles() {
        std::filesystem::remove( CURRENT_SOURCE_DIR"yamlFiles/linterFile_1.yaml" );
        std::filesystem::copy_file( CURRENT_SOURCE_DIR"yamlFiles/linterFile_1_save.yaml",
                                    CURRENT_SOURCE_DIR"yamlFiles/linterFile_1.yaml" );
        std::filesystem::remove( CURRENT_SOURCE_DIR"yamlFiles/linterFile_2.yaml" );
        std::filesystem::copy_file( CURRENT_SOURCE_DIR"yamlFiles/linterFile_2_save.yaml",
                                    CURRENT_SOURCE_DIR"yamlFiles/linterFile_2.yaml" );
    }
};

// Is we need this class? - we have diagnostics now
class StreamCapture {
public:
    explicit StreamCapture( std::ostream & stream ) : stream( &stream ) {
        old = stream.flush().rdbuf( buffer.rdbuf() );
    }

    ~StreamCapture() {
        buffer.flush();
        stream->rdbuf( old );
    }

    std::string getBufferData() const {
        return buffer.str();
    }

private:
    std::ostream * stream;
    std::ostringstream buffer;
    std::streambuf * old;
};

namespace LintCombine {
    struct MockLinterWrapper : LinterBase {
        MockLinterWrapper( const stringVector & commandLine, LinterFactoryBase::Services & service ) : LinterBase( service ) {
            name = commandLine[1];
            if( commandLine.size() >= 3 ) {
                yamlPath = commandLine[2];
            }
        }

        void updateYamlAction( const YAML::Node & ) const override {}
    };

    struct MocksLinterFactory : LinterFactoryBase {
        MocksLinterFactory( const MocksLinterFactory & ) = delete;

        MocksLinterFactory( MocksLinterFactory && ) = delete;

        MocksLinterFactory & operator=( MocksLinterFactory const & ) = delete;

        MocksLinterFactory & operator=( MocksLinterFactory const && ) = delete;

        static MocksLinterFactory & getInstance() {
            static MocksLinterFactory mockFactory;
            return mockFactory;
        }

        std::shared_ptr < LinterItf >
            createLinter( const stringVector & commandLineSTL ) override {
            if( commandLineSTL[0] == "MockLinterWrapper" ) {
                return std::make_shared < MockLinterWrapper >( commandLineSTL, getServices() );
            }
            return nullptr;
        }

    private:
        MocksLinterFactory() = default;
    };
}

BOOST_AUTO_TEST_SUITE( TestLinterCombineConstructor )

BOOST_AUTO_TEST_CASE( ClangTidyAndClazyExist ) {
    const LintCombine::stringVector cmdLine = {
        "--sub-linter=clang-tidy", "--sub-linter=clazy" };
    const LintCombine::LinterCombine combine( cmdLine );
    BOOST_REQUIRE( combine.numLinters() == 2 );
    const auto & linter_1 = std::dynamic_pointer_cast < LintCombine::LinterBase >
        ( combine.linterAt( 0 ) );
    const auto & linter_2 = std::dynamic_pointer_cast < LintCombine::LinterBase >
        ( combine.linterAt( 1 ) );
    BOOST_CHECK( linter_1->getName() == "clang-tidy" );
    BOOST_CHECK( linter_1->getOptions().empty() );
    BOOST_CHECK( linter_1->getYamlPath().empty() );
    BOOST_CHECK( linter_2->getName() == "clazy-standalone" );
    BOOST_CHECK( linter_2->getOptions().empty() );
    BOOST_CHECK( linter_2->getYamlPath().empty() );
}

BOOST_AUTO_TEST_CASE( ClangTidyAndClazyExistAndHaveOptions ) {
    const LintCombine::stringVector cmdLine = {
        "--sub-linter=clang-tidy", "CTParam_1", "CTParam_2",
        "--sub-linter=clazy", "CSParam_1", "CSParam_1" };
    const LintCombine::LinterCombine combine( cmdLine );
    BOOST_REQUIRE( combine.numLinters() == 2 );
    const auto & linter_1 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 0 ) );
    const auto & linter_2 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 1 ) );
    BOOST_CHECK( linter_1->getName() == "clang-tidy" );
    BOOST_CHECK( linter_1->getOptions() == "CTParam_1 CTParam_2 " );
    BOOST_CHECK( linter_1->getYamlPath().empty() );
    BOOST_CHECK( linter_2->getName() == "clazy-standalone" );
    BOOST_CHECK( linter_2->getOptions() == "CSParam_1 CSParam_1 " );
    BOOST_CHECK( linter_2->getYamlPath().empty() );
}

BOOST_AUTO_TEST_CASE( ClazyStandaloneExists ) {
    const LintCombine::stringVector cmdLine = { "--sub-linter=clazy" };
    const LintCombine::LinterCombine combine( cmdLine );
    BOOST_REQUIRE( combine.numLinters() == 1 );
    const auto & linter_1 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 0 ) );

    BOOST_CHECK( linter_1->getName() == "clazy-standalone" );
    BOOST_CHECK( linter_1->getOptions().empty() );
    BOOST_CHECK( linter_1->getYamlPath().empty() );
}

BOOST_AUTO_TEST_CASE( ClangTidyExists ) {
    const LintCombine::stringVector cmdLine = { "--sub-linter=clang-tidy" };
    const LintCombine::LinterCombine combine( cmdLine );
    BOOST_REQUIRE( combine.numLinters() == 1 );
    const auto & linter_1 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 0 ) );

    BOOST_CHECK( linter_1->getName() == "clang-tidy" );
    BOOST_CHECK( linter_1->getOptions().empty() );
    BOOST_CHECK( linter_1->getYamlPath().empty() );
}

BOOST_AUTO_TEST_CASE( ClazyStandaloneExistsAndHasOptions ) {
    const LintCombine::stringVector commandLineSTL = {
        "--sub-linter=clazy", "lintParam_1", "lintParam_2" };
    const LintCombine::LinterCombine linterCombine( commandLineSTL );
    BOOST_CHECK( linterCombine.numLinters() == 1 );
    BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                         ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
    BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                         ( linterCombine.linterAt( 0 ) )->getOptions() == "lintParam_1 lintParam_2 " );
    BOOST_CHECK( linterCombine.linterAt( 0 )->getYamlPath().empty() );
}

BOOST_AUTO_TEST_CASE( LinterIsClazyStandaloneAndYamlPathExists ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=clazy",
                                                 "--export-fixes="
                                                 CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
    const LintCombine::LinterCombine linterCombine( commandLineSTL );
    BOOST_CHECK( linterCombine.numLinters() == 1 );
    BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                         ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
    BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                         ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
    BOOST_CHECK( linterCombine.linterAt( 0 )->getYamlPath() == CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" );
}

BOOST_AUTO_TEST_CASE( LinterIsClazyStandaloneAndOptionsAndYamlPathExist ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=clazy",
                                                 "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml",
                                                 "lintParam_1", "lintParam_2" };
    const LintCombine::LinterCombine linterCombine( commandLineSTL );
    BOOST_CHECK( linterCombine.numLinters() == 1 );
    BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                         ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
    BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                         ( linterCombine.linterAt( 0 ) )->getOptions() == "lintParam_1 lintParam_2 " );
    BOOST_CHECK( linterCombine.linterAt( 0 )->getYamlPath() == CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" );
}

BOOST_AUTO_TEST_CASE( BothLintersExistFirstYamlPathNotSetSecondYamlPathIsEmpty ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy",
                                                 "--sub-linter=clazy", "--export-fixes= " };
    const LintCombine::LinterCombine linterCombine( commandLineSTL );
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
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy", "--export-fixes=\\\\",
                                                 "--sub-linter=clazy",
                                                 "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
    const LintCombine::LinterCombine linterCombine( commandLineSTL );
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
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy",
                                                 "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml",
                                                 "CTParam_1", "CTParam_2",
                                                 "--sub-linter=clazy",
                                                 "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_2.yaml",
                                                 "CSParam_1",
                                                 "CSParam_2" };
    const LintCombine::LinterCombine linterCombine( commandLineSTL );
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
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back(
                "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh" );
    }

    LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
    linterCombine.callLinter();
    int linterCombineReturnCode = linterCombine.waitLinter();
    std::string stdoutData = stdoutCapture.getBufferData();
    BOOST_CHECK( linterCombineReturnCode == 3 );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
}

BOOST_AUTO_TEST_CASE( LinterReturn1 ) {
    StreamCapture stdoutCapture( std::cout );
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.sh" );
    }

    LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
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
    LintCombine::stringVector cmdLine = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        cmdLine.emplace_back(
                "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh" );
        cmdLine.emplace_back( "--sub-linter=MockLinterWrapper" );
        cmdLine.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        cmdLine.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh" );
        cmdLine.emplace_back( "--sub-linter=MockLinterWrapper" );
        cmdLine.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.sh" );
    }

    LintCombine::LinterCombine linterCombine( cmdLine, LintCombine::MocksLinterFactory::getInstance() );
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
    LintCombine::stringVector cmdLine = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        cmdLine.emplace_back(
                "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh" );
        cmdLine.emplace_back( "--sub-linter=MockLinterWrapper" );
        cmdLine.emplace_back(
                "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_2.sh" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        cmdLine.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh" );
        cmdLine.emplace_back( "--sub-linter=MockLinterWrapper" );
        cmdLine.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_2.sh" );
    }

    LintCombine::LinterCombine linterCombine( cmdLine, LintCombine::MocksLinterFactory::getInstance() );
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
    LintCombine::stringVector cmdLine = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        cmdLine.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.bat" );
        cmdLine.emplace_back( "--sub-linter=MockLinterWrapper" );
        cmdLine.emplace_back( CURRENT_SOURCE_DIR
                                     "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        cmdLine.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.sh" );
        cmdLine.emplace_back( "--sub-linter=MockLinterWrapper" );
        cmdLine.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.sh" );
    }

    LintCombine::LinterCombine linterCombine( cmdLine, LintCombine::MocksLinterFactory::getInstance() );
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
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.bat" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_2.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.sh" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_2.sh" );
    }

    LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
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
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.sh" );
    }

    LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
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
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_1.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_1.sh" );
    }

    LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
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
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_1.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockReturn0WriteToStreamsWriteToFile_1.sh" );
    }

    LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
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
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.bat" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_2.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.sh" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_2.sh" );
    }

    LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
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
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_1.bat" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_2.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_1.sh" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_2.sh" );
    }

    LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
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
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.bat" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_2.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.sh" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_2.sh" );
    }

    LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
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
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR
                                     "mockPrograms/mockReturn0WriteToStreamsWriteToFile_1.bat" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR
                                     "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockReturn0WriteToStreamsWriteToFile_1.sh" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.sh" );
    }

    LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
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
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_1.sh" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_2.sh" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_1.sh" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_2.sh" );
    }

    LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
    linterCombine.callLinter();
    int linterCombineReturnCode = linterCombine.waitLinter();
    std::string stdoutData = stdoutCapture.getBufferData();
    BOOST_CHECK( linterCombineReturnCode == 0 );
    BOOST_CHECK( stdoutData == "First_stdout_mes_1\nSecond_stdout_mes_1\nFirst_stdout_mes_2\n" );
}

BOOST_AUTO_TEST_CASE( OneLinterEndsEarlierThatCombine ) {
    std::ofstream( CURRENT_SOURCE_DIR "file_1.txt" );
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
                                     CURRENT_SOURCE_DIR "file_1.txt" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
                                     CURRENT_SOURCE_DIR "file_1.txt" );
    }

    LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
    linterCombine.callLinter();
    BOOST_CHECK( linterCombine.waitLinter() == 0 );
    BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR "file_1.txt" ) );
    std::filesystem::remove( CURRENT_SOURCE_DIR "file_1.txt" );
}

BOOST_AUTO_TEST_CASE( TwoLinterEndsEarlierThatCombine ) {
    std::ofstream( CURRENT_SOURCE_DIR "file_1.txt" );
    std::ofstream( CURRENT_SOURCE_DIR "file_2.txt" );
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
                                     CURRENT_SOURCE_DIR"file_1.txt" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.3 "
                                     CURRENT_SOURCE_DIR"file_2.txt" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
                                     CURRENT_SOURCE_DIR "file_1.txt" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.3 "
                                     CURRENT_SOURCE_DIR "file_2.txt" );
    }

    LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
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
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper", "defaultLinter",
                                                 "NotExistentFile" };
    const LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
    const LintCombine::CallTotals callTotals = linterCombine.updateYaml();
    BOOST_CHECK( callTotals.successNum == 0 );
    BOOST_CHECK( callTotals.failNum == 1 );
}

BOOST_AUTO_TEST_CASE( EmptyYamlPath ) {
    const LintCombine::stringVector commandLineSTL = { "", "--sub-linter=MockLinterWrapper", "defaultLinter", "" };
    const LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
    const LintCombine::CallTotals callTotals = linterCombine.updateYaml();
    BOOST_CHECK( callTotals.successNum == 0 );
    BOOST_CHECK( callTotals.failNum == 1 );
}

BOOST_AUTO_TEST_CASE( FirstHasEmptyYamlPathSecondHasPathToExistsYaml ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper", "defaultLinter", "",
                                                 "--sub-linter=MockLinterWrapper", "defaultLinter",
                                                 CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" };
    const LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
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
    const LintCombine::stringVector commandLineSTL = { "", "--sub-linter=MockLinterWrapper", "defaultLinter", "",
                                                 "--sub-linter=MockLinterWrapper", "defaultLinter", "" };
    const LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
    const LintCombine::CallTotals callTotals = linterCombine.updateYaml();
    BOOST_CHECK( callTotals.successNum == 0 );
    BOOST_CHECK( callTotals.failNum == 2 );
}

BOOST_AUTO_TEST_CASE( FirstHasNotExistentYamlPathSecondHasPathToExistsYaml ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper", "defaultLinter", "NotExistentFile",
                                                 "--sub-linter=MockLinterWrapper", "defaultLinter",
                                                 CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" };
    const LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
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
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper", "defaultLinter", "NotExistentFile",
                                                 "--sub-linter=MockLinterWrapper", "defaultLinter", "NotExistentFile" };
    const LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
    const LintCombine::CallTotals callTotals = linterCombine.updateYaml();
    BOOST_CHECK( callTotals.successNum == 0 );
    BOOST_CHECK( callTotals.failNum == 2 );
}

BOOST_AUTO_TEST_CASE( YamlPathExists ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper", "defaultLinter",
                                                 CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml" };
    const LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
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
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper", "defaultLinter",
                                                 CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml",
                                                 "--sub-linter=MockLinterWrapper", "defaultLinter",
                                                 CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" };
    LintCombine::LinterCombine linterCombine( commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
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

BOOST_FIXTURE_TEST_CASE( clangTidyTest, recoverYamlFiles ) {
    LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy",
                                                 "--export-fixes="
                                                 CURRENT_SOURCE_DIR "/yamlFiles/linterFile_1.yaml" };
    LintCombine::LinterCombine linterCombine( commandLineSTL );
    LintCombine::CallTotals callTotals = linterCombine.updateYaml();
    BOOST_CHECK( callTotals.successNum == 1 );
    BOOST_CHECK( callTotals.failNum == 0 );

    std::ifstream yamlFile_1( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml" );
    std::ifstream yamlFile_1_save( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1_result.yaml" );
    std::istream_iterator < char > fileIter_1( yamlFile_1 ), end_1;
    std::istream_iterator < char > fileIter_1_save( yamlFile_1_save ), end_1_save;
    BOOST_CHECK_EQUAL_COLLECTIONS( fileIter_1, end_1, fileIter_1_save, end_1_save );
}

BOOST_FIXTURE_TEST_CASE( clazyTest, recoverYamlFiles ) {
    LintCombine::stringVector cmdLine = { "--sub-linter=clazy",
                                                 "--export-fixes="
                                                 CURRENT_SOURCE_DIR "/yamlFiles/linterFile_2.yaml" };
    LintCombine::LinterCombine linterCombine( cmdLine );
    LintCombine::CallTotals callTotals = linterCombine.updateYaml();
    BOOST_CHECK( callTotals.successNum == 1 );
    BOOST_CHECK( callTotals.failNum == 0 );

    std::ifstream yamlFile_2( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" );
    std::ifstream yamlFile_2_save( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2_result.yaml" );
    std::istream_iterator < char > fileIter_2( yamlFile_2 ), end_2;
    std::istream_iterator < char > fileIter_2_save( yamlFile_2_save ), end_2_save;
    BOOST_CHECK_EQUAL_COLLECTIONS( fileIter_2, end_2, fileIter_2_save, end_2_save );
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestMergeYaml )

BOOST_AUTO_TEST_CASE( mergedYamlPathIsEmptyLintersYamlPathValid ) {
    const LintCombine::stringVector commandLineSTL = { "--result-yaml=", "--sub-linter=clang-tidy",
                                                 "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
    BOOST_CHECK_THROW( LintCombine::LinterCombine{ commandLineSTL }, boost::program_options::error );
}

BOOST_AUTO_TEST_CASE( mergedYamlPathSetLintersYamlPathValid ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy",
                                                 "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
    LintCombine::LinterCombine linterCombine( commandLineSTL );
    BOOST_CHECK( linterCombine.getYamlPath().empty() );
}

BOOST_AUTO_TEST_CASE( mergedYamlPathNotValidLintersYamlPathValid ) {
    const LintCombine::stringVector commandLineSTL = { "--result-yaml=\\\\", "--sub-linter=clang-tidy",
                                                 "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
    LintCombine::LinterCombine linterCombine( commandLineSTL );
    std::cout << "\"" << linterCombine.getYamlPath() << "\"" << std::endl;
    BOOST_CHECK( linterCombine.getYamlPath().empty() );
}

BOOST_AUTO_TEST_CASE( mergedYamlPathNotSetLintersYamlPathNotSet ) {
    const LintCombine::stringVector commandLineSTL = { "", "--sub-linter=clang-tidy" };
    LintCombine::LinterCombine linterCombine( commandLineSTL );
    BOOST_CHECK( linterCombine.getYamlPath().empty() );
}

BOOST_AUTO_TEST_CASE( mergedYamlValidLintersYamlPathNotSet ) {
    const LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                 CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                 "--sub-linter=clang-tidy" };
    LintCombine::LinterCombine linterCombine( commandLineSTL );
    BOOST_CHECK( linterCombine.getYamlPath().empty() );
    BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" ) );
}

BOOST_AUTO_TEST_CASE( mergedYamlValidLintersYamlPathIsEmpty ) {
    const LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                 CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                 "--sub-linter=clang-tidy", "--export-fixes=" };
    BOOST_CHECK_THROW( LintCombine::LinterCombine{ commandLineSTL }, boost::program_options::error );
}

BOOST_AUTO_TEST_CASE( mergedYamlValidLintersYamlPathNotValid ) {
    const LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                 CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                 "--sub-linter=clang-tidy", "--export-fixes=\\\\" };
    LintCombine::LinterCombine linterCombine( commandLineSTL );
    BOOST_CHECK( linterCombine.getYamlPath().empty() );
    BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR"yamlFiles/combinedResult.yaml" ) );
}

BOOST_AUTO_TEST_CASE( mergedYamlValidLintersYamlPathNotExists ) {
    const LintCombine::stringVector commandLineSTL = { "--result-yaml="
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
    const LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                 CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                 "--sub-linter=clang-tidy",
                                                 "--sub-linter=clang-tidy" };
    LintCombine::LinterCombine linterCombine( commandLineSTL );
    BOOST_CHECK( linterCombine.getYamlPath().empty() );
}

BOOST_AUTO_TEST_CASE( mergedYamlValidFirstLintersYamlPathIsEmptySecondValid ) {
    const LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                 CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                 "--sub-linter=clang-tidy", "--export-fixes=",
                                                 "--sub-linter=clang-tidy",
                                                 "--export-fixes="
                                                 CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" };
    BOOST_CHECK_THROW( LintCombine::LinterCombine{ commandLineSTL }, boost::program_options::error );
}

BOOST_AUTO_TEST_CASE( mergedYamlValidBothLintersYamlPathIsEmpty ) {
    const LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                 CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                 "--sub-linter=clang-tidy", "--export-fixes=",
                                                 "--sub-linter=clang-tidy", "--export-fixes=" };
    BOOST_CHECK_THROW( LintCombine::LinterCombine{ commandLineSTL }, boost::program_options::error );
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
    const LintCombine::stringVector commandLineSTL = { "--result-yaml="
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
    const LintCombine::stringVector commandLineSTL = { "--result-yaml="
                                                 CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
                                                 "--sub-linter=clang-tidy", "--export-fixes=NotExistentFile",
                                                 "--sub-linter=clang-tidy", "--export-fixes=NotExistentFile" };
    LintCombine::LinterCombine linterCombine( commandLineSTL );
    BOOST_CHECK( linterCombine.getYamlPath().empty() );
}

BOOST_AUTO_TEST_CASE( mergedYamlValidBothLintersYamlPathValid ) {
    const LintCombine::stringVector commandLineSTL = { "--result-yaml="
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

// TODO: Test print diagnostics separetely

// TODO: Figure out: why memory leak occurs in Debug
BOOST_AUTO_TEST_SUITE( TestPrepareCommandLine )

template < int T >
void compareContainers( const LintCombine::stringVector & lhs, const std::array < std::string, T > & rhs ) {
    BOOST_REQUIRE( lhs.size() == rhs.size() );
    for( size_t i = 0; i < lhs.size(); ++i ) {
        BOOST_CHECK( lhs[i] == rhs[i] );
    }
}

BOOST_AUTO_TEST_CASE( EmptyCommandLine ) {
    LintCombine::stringVector cmdLine = {};
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Command line is empty" );
}

BOOST_AUTO_TEST_CASE( FactoryDeleteIdeProfile_ValueAfterEqualSign ) {
    LintCombine::stringVector cmdLine = {
        "--param=value", "--ide-profile=resharper", "--param=value" };
    LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    // constexpr std::array< char *, 2 > result = { "--param=value" ,  "--param=value" };
    const std::array< std::string, 2 > result = { "--param=value" ,  "--param=value" };
    compareContainers( cmdLine, result );
}

BOOST_AUTO_TEST_CASE( FactoryDeleteIdeProfile_ValueAfterSpace ) {
    LintCombine::stringVector cmdLine = {
        "--param=value", "--ide-profile", "resharper", "--param=value" };
    LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    const std::array< std::string, 2 > result = { "--param=value", "--param=value" };
    compareContainers( cmdLine, result );
}

BOOST_AUTO_TEST_CASE( VerbatimLintersDontExist ) {
    LintCombine::stringVector cmdLine = {
        "--param=value", "-p=val", "--param", "val" };
    auto * prepareCmdLine =
        LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "No linters specified" );
}

BOOST_AUTO_TEST_CASE( VerbatimOneLinterWithIncorrectName ) {
    LintCombine::stringVector cmdLine = {
        "--sub-linter=Incorrect", "--param=value", "-p=val", "--param", "val" };
    auto * prepareCmdLine =
        LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Unknown linter name: \"Incorrect\"" );
}

BOOST_AUTO_TEST_CASE( VerbatimTwoLintersWithIncorrectNames ) {
    LintCombine::stringVector cmdLine = {
        "--sub-linter=Incorrect_1", "--sub-linter=Incorrect_2",
        "--param=value", "-p=val", "--param", "val" };
    auto * prepareCmdLine =
        LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Unknown linter name: \"Incorrect_1\"" );
    const auto diagnostic_1 = prepareCmdLine->diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text == "Unknown linter name: \"Incorrect_2\"" );
}

BOOST_AUTO_TEST_CASE( VerbatimOneLinterWithCorrectName ) {
    LintCombine::stringVector cmdLine = {
        "--result-yaml=file.yaml", "--sub-linter=clazy",
        "--param=value", "-p=val", "--param", "val" };
    const std::array < std::string, 7 > result = {
        "--result-yaml=file.yaml", "--sub-linter=clazy",
        "--param=value", "-p=val", "--param", "val" };
    auto * prepareCmdLine =
        LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( VerbatimTwoLintersWithCorrectNames ) {
    LintCombine::stringVector cmdLine = {
        "--result-yaml=file.yaml", "--sub-linter=clazy",
        "--sub-linter=clang-tidy", "--param=value",
        "-p=val", "--param", "val" };
    const std::array < std::string, 7 > result = {
        "--result-yaml=file.yaml", "--sub-linter=clazy",
        "--sub-linter=clang-tidy", "--param=value",
        "-p=val", "--param", "val" };
    auto * prepareCmdLine =
        LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( VerbatimResultYamlPathNotExists ) {
    LintCombine::stringVector cmdLine = {
        "--sub-linter=clazy", "--param=value", "-p=val", "--param", "val" };
    const std::array < std::string, 7 > result = {
        "--result-yaml=" CURRENT_BINARY_DIR "LintersDiagnostics.yaml",
        "--sub-linter=clazy", "--param=value",
        "-p=val", "--param", "val" };
    auto * prepareCmdLine =
        LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "result-yaml path doesn't set." );
}

BOOST_AUTO_TEST_CASE( VerbatimInvalidResultYamlPath ) {
    LintCombine::stringVector cmdLine = {
        "--result-yaml=\\\\", "--sub-linter=clazy",
        "--param=value", "-p=val", "--param", "val" };
    const std::array < std::string, 7 > result = {
        "--result-yaml=" CURRENT_BINARY_DIR "LintersDiagnostics.yaml",
        "--sub-linter=clazy", "--param=value",
        "-p=val", "--param", "val" };
    auto * prepareCmdLine =
        LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "result-yaml path is invalid." );
}

BOOST_AUTO_TEST_CASE( UnsupportedIDE ) {
    LintCombine::stringVector cmdLine = { "--ide-profile=shasharper" };
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ) == cmdLine );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "\"shasharper\" isn't supported by cpp-lint-combine" );
}

BOOST_AUTO_TEST_CASE( SpecifiedTwice ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase", "-p=pathToCompilationDataBase" };
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "option '--p' cannot be specified more than once" );
}

BOOST_AUTO_TEST_CASE( PathToYamlFileIsEmpty ) {
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase" };
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Path to yaml-file is empty." );
}

BOOST_AUTO_TEST_CASE( PathToComilationDataBaseIsEmpty ) {
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-export-fixes=pathToResultYaml" };
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Path to compilation database is empty." );
}

BOOST_AUTO_TEST_CASE( MinimalRequiredOptionsExist ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase", "--export-fixes=pathToResultYaml" };
    const std::array < std::string, 7 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( OptionForClangTidy ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--param_1", "@param_2" };

    const std::array < std::string, 9 > result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
        "--param_1", "@param_2",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( FilesToAnalizeAreSet ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "file_1.cpp", "file_2.cpp" };
    const std::array < std::string, 11 > result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
        "file_1.cpp", "file_2.cpp",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml",
        "file_1.cpp", "file_2.cpp" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( HeaderFilterSet ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--header-filter=file.cpp" };

    const std::array < std::string, 9 > result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
        "--header-filter=file.cpp",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml",
        "--header-filter=file.cpp" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( ClazyChecksEmptyAfterEqualSign ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--clazy-checks=" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the argument for option '--clazy-checks'"
                                      " should follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( ClangExtraArgsEmptyAfterEqualSign ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--clang-extra-args=" };
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the argument for option '--clang-extra-args'"
                                      " should follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( AllParamsEmptyAfterEqualSign ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--clazy-checks=", "--clang-extra-args=" };
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the argument for option '--clazy-checks'"
                                      " should follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( ClazyChecksEmptyAfterSpace ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--clazy-checks" };
    const std::array < std::string, 7 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_0.firstPos == 63 );
    BOOST_CHECK( diagnostic_0.lastPos == 75 );
    BOOST_CHECK( diagnostic_0.text == "Parameter \"clazy-checks\" was set but "
                                      "the parameter's value was not set. "
                                      "The parameter will be ignored." );
}

BOOST_AUTO_TEST_CASE( ClangExtraArgsEmptyAfterSpace ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--clang-extra-args" };
    const std::array < std::string, 7 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_0.firstPos == 63 );
    BOOST_CHECK( diagnostic_0.lastPos == 79 );
    BOOST_CHECK( diagnostic_0.text == "Parameter \"clang-extra-args\" was set but "
                                      "the parameter's value was not set. "
                                      "The parameter will be ignored." );
}

BOOST_AUTO_TEST_CASE( AllParamsEmptyAfterSpace ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
                                          "--export-fixes=pathToResultYaml",
                                          "--clazy-checks", "--clang-extra-args" };
    const std::array < std::string, 7 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 2 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_0.firstPos == 63 );
    BOOST_CHECK( diagnostic_0.lastPos == 75 );
    BOOST_CHECK( diagnostic_0.text == "Parameter \"clazy-checks\" was set but "
                                      "the parameter's value was not set. "
                                      "The parameter will be ignored." );
    const auto diagnostic_1 = prepareCmdLine->diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_1.firstPos == 78 );
    BOOST_CHECK( diagnostic_1.lastPos == 94 );
    BOOST_CHECK( diagnostic_1.text == "Parameter \"clang-extra-args\" was set but "
                                      "the parameter's value was not set. "
                                      "The parameter will be ignored." );
}

BOOST_AUTO_TEST_CASE( ClazyChecksExist ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--clazy-checks", "level0,level1" };
    const std::array < std::string, 8 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml",
            "--checks=level0,level1" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( ClangExtraArgsExist ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--clang-extra-args=arg_1 arg_2 " };
    const std::array < std::string, 9 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml",
            "--extra-arg=arg_1", "--extra-arg=arg_2" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( AllParamsExistAfterEqualSign ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--clazy-checks=level0,level1",
                                              "--clang-extra-args=arg_1 arg_2" };
    const std::array < std::string, 10 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml",
            "--checks=level0,level1",
            "--extra-arg=arg_1", "--extra-arg=arg_2" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( AllParamsExistAfterSpace ) {
    LintCombine::stringVector cmdLine
        = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
            "--export-fixes=pathToResultYaml",
            "--clazy-checks", "level0,level1",
            "--clang-extra-args", "arg_1 arg_2" };
    const std::array < std::string, 10 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml",
            "--checks=level0,level1",
            "--extra-arg=arg_1", "--extra-arg=arg_2" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( OneSublinterValueEmptyAfterEqualSign ) {
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
                                          "--export-fixes=pathToResultYaml",
                                          "--sub-linter=" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the argument for option '--sub-linter' should"
                                      " follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( FirstSubLinterIncorrectSecondValueEmptyAfterEqualSign ) {
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
                                          "--export-fixes=pathToResultYaml",
                                          "--sub-linter=IncorrectName_1", "--sub-linter=" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the argument for option '--sub-linter' should"
                                      " follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( FirstSubLinterValueEmptyAfterEqualSignSecondIncorrect ) {
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
                                          "--export-fixes=pathToResultYaml",
                                          "--sub-linter=", "--sub-linter=IncorrectName_1" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the argument for option '--sub-linter' should"
                                      " follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( TwoSublinterValuesEmptyAfterEqualSign ) {
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
                                          "--export-fixes=pathToResultYaml",
                                          "--sub-linter=", "--sub-linter=" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the argument for option '--sub-linter' should"
                                      " follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( OneSublinterNoValue ) {
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
                                          "--export-fixes=pathToResultYaml",
                                          "--sub-linter" };
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the required argument for option '--sub-linter' is missing" );
}

BOOST_AUTO_TEST_CASE( FirstSubLinterIncorrectSecondHasEmptyValueAfterSpace ) {
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
                                          "--export-fixes=pathToResultYaml",
                                          "--sub-linter", "IncorrectName_1", "--sub-linter" };
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the required argument for option '--sub-linter' is missing" );
}

BOOST_AUTO_TEST_CASE( FirstSubLinterHasEmptyValueAfterSpaceSecondIncorrect ) {
    LintCombine::stringVector cmdLine = { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
                                          "--export-fixes=pathToResultYaml",
                                          "--sub-linter", "--sub-linter", "IncorrectName_1" };
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    //-p=pathToCompilationDataBase --export-fixes=pathToResultYaml --sub-linter --sub-linter IncorrectName_1
    BOOST_CHECK( diagnostic_0.firstPos == 74 );
    BOOST_CHECK( diagnostic_0.lastPos == 86 );
    BOOST_CHECK( diagnostic_0.text == "Unknown linter name \"--sub-linter\"" );
}

BOOST_AUTO_TEST_CASE( AllSublintersHaveEmptyValueAfterSpace ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml",
        "--sub-linter", "--sub-linter" };
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 74 );
    BOOST_CHECK( diagnostic_0.lastPos == 86 );
    BOOST_CHECK( diagnostic_0.text == "Unknown linter name \"--sub-linter\"" );
}

BOOST_AUTO_TEST_CASE( OneSublinterWithIncorrectName ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=IncorrectName_1" };
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 74 );
    BOOST_CHECK( diagnostic_0.lastPos == 89 );
    BOOST_CHECK( diagnostic_0.text == "Unknown linter name \"IncorrectName_1\"" );
}

BOOST_AUTO_TEST_CASE( TwoSublinterWithIncorrectName ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=IncorrectName_1",
        "--sub-linter=IncorrectName_2" };
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    BOOST_CHECK( prepareCmdLine->transform( cmdLine ).empty() == true );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 2 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.firstPos == 74 );
    BOOST_CHECK( diagnostic_0.lastPos == 89 );
    BOOST_CHECK( diagnostic_0.text == "Unknown linter name \"IncorrectName_1\"" );
    const auto diagnostic_1 = prepareCmdLine->diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.firstPos == 103 );
    BOOST_CHECK( diagnostic_1.lastPos == 118 );
    BOOST_CHECK( diagnostic_1.text == "Unknown linter name \"IncorrectName_2\"" );
}

BOOST_AUTO_TEST_CASE( SublinterIsClangTidy ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=clang-tidy" };
    const std::array < std::string, 4 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( SublinterIsClazy ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=clazy" };
    const std::array < std::string, 4 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( SublintersAreClangTidyAndClazyAfterEqualSign ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=clang-tidy",
        "--sub-linter=clazy" };
    const std::array < std::string, 7 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( SublintersAreClangTidyAndClazyAfterSpace ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "-sub-linter", "clang-tidy",
        "-sub-linter", "clazy" };
    const std::array < std::string, 7 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };

    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transform( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_SUITE_END()
