#define BOOST_TEST_MODULE lintWrapperTesting

#include "../../src/LinterCombine.h"
#include "../../src/LinterBase.h"
#include "../../src/LintCombineUtils.h"

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

class StreamCapture {
public:
    explicit StreamCapture( std::ostream & stream ) : stream( &stream ) {
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
            name = commandLine[1];
            if( commandLine.size() >= 3 ) {
                yamlPath = commandLine[2];
            }
        }

        void updateYamlAction( const YAML::Node & ) const override {}
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
            if( commandLineSTL[0] == "MockWrapper" ) {
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
    BOOST_CHECK( str.find( "Product name" ) != std::string::npos );
    BOOST_CHECK( str.find( "Product version" ) != std::string::npos );
    BOOST_CHECK( str.find( "Program options" ) != std::string::npos );
    BOOST_CHECK( str.find( "Product name" ) != std::string::npos );
}

BOOST_AUTO_TEST_CASE( OneNotExistentLinter ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=NotExistentLinter" };
    BOOST_CHECK_THROW( LintCombine::LinterCombine{ commandLineSTL }, std::logic_error );
}

BOOST_AUTO_TEST_CASE( FirstLinterNotExistentSecondExists ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=NotExistentLinter", "--sub-linter=clazy" };
    BOOST_CHECK_THROW( LintCombine::LinterCombine{ commandLineSTL }, std::logic_error );
}

BOOST_AUTO_TEST_CASE( BothLintersNotExistent ) {
    const LintCombine::stringVector commandLineSTL = { "", "--sub-linter=NotExistentLinter_1",
                                                     "--sub-linter=NotExistentLinter_2" };
    BOOST_CHECK_THROW( LintCombine::LinterCombine{ commandLineSTL }, std::logic_error );
}

BOOST_AUTO_TEST_CASE( FirstLinterEmptySecondExists ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=", "--sub-linter=clazy" };
    BOOST_CHECK_THROW( LintCombine::LinterCombine{ commandLineSTL }, std::logic_error );
}

BOOST_AUTO_TEST_CASE( BothLintersEmpty ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=", "--sub-linter=" };
    BOOST_CHECK_THROW( LintCombine::LinterCombine{ commandLineSTL }, std::logic_error );
}

BOOST_AUTO_TEST_CASE( BothLintersExist ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy", "--sub-linter=clazy" };
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

BOOST_AUTO_TEST_CASE( BothLintersExistAndHasOptions ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy", "CTParam_1", "CTParam_2",
                                                 "--sub-linter=clazy", "CSParam_1", "CSParam_1" };
    const LintCombine::LinterCombine linterCombine( commandLineSTL );
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
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=clazy" };
    const LintCombine::LinterCombine linterCombine( commandLineSTL );
    BOOST_CHECK( linterCombine.numLinters() == 1 );
    BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                         ( linterCombine.linterAt( 0 ) )->getName() == "clazy-standalone" );
    BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                         ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
    BOOST_CHECK( linterCombine.linterAt( 0 )->getYamlPath().empty() );
}

BOOST_AUTO_TEST_CASE( LinterIsClangTidy ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=clang-tidy" };
    const LintCombine::LinterCombine linterCombine( commandLineSTL );
    BOOST_CHECK( linterCombine.numLinters() == 1 );
    BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                         ( linterCombine.linterAt( 0 ) )->getName() == "clang-tidy" );
    BOOST_CHECK( std::dynamic_pointer_cast < LintCombine::LinterBase >
                         ( linterCombine.linterAt( 0 ) )->getOptions().empty() );
    BOOST_CHECK( linterCombine.linterAt( 0 )->getYamlPath().empty() );
}

BOOST_AUTO_TEST_CASE( LinterIsClazyStandaloneAndOptionsExist ) {
    const LintCombine::stringVector commandLineSTL = { "--sub-linter=clazy", "lintParam_1",
                                                 "lintParam_2" };
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
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back(
                "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLine.emplace_back(
                "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh" );
        commandLine.emplace_back( "--sub-linter=MockWrapper" );
        commandLine.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLine.emplace_back(
                "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_1.sh" );
        commandLine.emplace_back( "--sub-linter=MockWrapper" );
        commandLine.emplace_back(
                "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockTerminatedWriteToStreams_2.sh" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLine.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.bat" );
        commandLine.emplace_back( "--sub-linter=MockWrapper" );
        commandLine.emplace_back( CURRENT_SOURCE_DIR
                                     "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_1.bat" );
        commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn1WriteToStreams_2.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_1.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreamsWriteToFile_1.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.bat" );
        commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_2.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_1.bat" );
        commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_2.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToStreams_1.bat" );
        commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR "mockPrograms/mockReturn0WriteToFile_2.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR
                                     "mockPrograms/mockReturn0WriteToStreamsWriteToFile_1.bat" );
        commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR
                                     "mockPrograms/mockReturn0WriteToStreamsWriteToFile_2.bat" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_1.sh" );
        commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
        commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_2.sh" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
                                     CURRENT_SOURCE_DIR "file_1.txt" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
                                     CURRENT_SOURCE_DIR"file_1.txt" );
        commandLineSTL.emplace_back( "--sub-linter=MockWrapper" );
        commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.3 "
                                     CURRENT_SOURCE_DIR"file_2.txt" );
    }
    if constexpr( BOOST_OS_LINUX ) {
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
    LintCombine::stringVector commandLine = { "--sub-linter=clazy",
                                                 "--export-fixes="
                                                 CURRENT_SOURCE_DIR "/yamlFiles/linterFile_2.yaml" };
    LintCombine::LinterCombine linterCombine( commandLine );
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

// TODO: Figure out: why memory leak occurs in Debug
// TODO: Rename syntax error to emptyValueWithoutEquelSign
BOOST_AUTO_TEST_SUITE( TestPrepareCommandLine )

template < int T >
void compareVectors( const LintCombine::stringVector & lhs, const std::array < std::string, T > & rhs ) {
    BOOST_REQUIRE( lhs.size() == rhs.size() );
    for( size_t i = 0; i < lhs.size(); ++i ) {
        BOOST_CHECK( lhs[i] == rhs[i] );
    }
}

BOOST_AUTO_TEST_CASE( TestEmptyCommandLine ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = {};
    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == true );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData() == "Error: Command line is empty.\n" );
}

BOOST_AUTO_TEST_CASE( TestSpecifiedTwice ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "-p=pathToCompilationDataBase" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == true );
    std::cout << stderrCapture.getBufferData() << std::endl;
    BOOST_CHECK( stderrCapture.getBufferData() == "Error: option '--p' cannot be specified more than once\n" );
}

BOOST_AUTO_TEST_CASE( TestVerbatimOptionExists ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "--verbatim-commands", "param1",
                                              "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml", "param2" };
    const std::array < std::string, 4 > result = { "param1", "-p=pathToCompilationDataBase",
                                                "--export-fixes=pathToResultYaml", "param2" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData() ==
                    "Info: Option \"--verbatim-commands\" "
                    "was set so options are passed to combine verbatim.\n"
                    "--verbatim-commands param1 -p=pathToCompilationDataBase "
                    "--export-fixes=pathToResultYaml param2\n"
                    "~~~~~~~~~~~~~~~~~~~\n" );
}

BOOST_AUTO_TEST_CASE( TestPathToYamlFileIsEmpty ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase" };
    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == true );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData() == "Error: Path to yaml-file is empty.\n" );
}

BOOST_AUTO_TEST_CASE( TestPathToComilationDataBaseIsEmpty ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-export-fixes=pathToResultYaml" };
    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == true );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData() == "Error: Path to compilation database is empty.\n" );
}

BOOST_AUTO_TEST_CASE( TestMinimalRequiredOptionsExist ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml" };
    const std::array < std::string, 7 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    BOOST_CHECK( stderrCapture.getBufferData().empty() );
}

BOOST_AUTO_TEST_CASE( TestOptionForClangTidy ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "-param_1", "@param_2" };

    const std::array < std::string, 9 > result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
        "-param_1", "@param_2",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    BOOST_CHECK( stderrCapture.getBufferData().empty() );
}

BOOST_AUTO_TEST_CASE( TestFilesToAnalizeAreSet ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "file_1.cpp", "file_2.cpp" };
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

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    BOOST_CHECK( stderrCapture.getBufferData().empty() );
}

BOOST_AUTO_TEST_CASE( TestHeaderFilterSet ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "-header-filter=file.cpp" };

    const std::array < std::string, 9 > result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
        "-header-filter=file.cpp",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml",
        "-header-filter=file.cpp" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    BOOST_CHECK( stderrCapture.getBufferData().empty() );
}

BOOST_AUTO_TEST_CASE( TestClazyChecksEmptyAfterEqualSign ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--clazy-checks=" };
    const std::array < std::string, 7 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    std::string strClazyChecksUnderLine( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml " ).size(), ' ' );
    strClazyChecksUnderLine.append( std::string( "--clazy-checks=" ).size(), '~' );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData() ==
                 "Warning: Parameter \"clazy-checks\" was "
                 "set but the parameter's value was not set. "
                 "The parameter will be ignored.\n"
                 "-p=pathToCompilationDataBase "
                 "--export-fixes=pathToResultYaml "
                 "--clazy-checks=\n" + strClazyChecksUnderLine + "\n" );
}

BOOST_AUTO_TEST_CASE( TestClangExtraArgsEmptyAfterEqualSign ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--clang-extra-args=" };
    const std::array < std::string, 7 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    std::string strClangArgsUnderline( std::string( "-p=pathToCompilationDataBase "
                                       "--export-fixes=pathToResultYaml " ).size(), ' ' );
    strClangArgsUnderline.append( std::string( "--clang-extra-args=" ).size(), '~' );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData() ==
                 "Warning: Parameter \"clang-extra-args\" was "
                  "set but the parameter's value was not set. "
                  "The parameter will be ignored.\n"
                  "-p=pathToCompilationDataBase "
                  "--export-fixes=pathToResultYaml "
                  "--clang-extra-args=\n" + strClangArgsUnderline + "\n" );
}

BOOST_AUTO_TEST_CASE( TestAllParamsHaveEmptyValue ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--clazy-checks=", "--clang-extra-args=" };
    const std::array < std::string, 7 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    std::string strClazyChecksUnderline( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml " ).size(), ' ' );
    strClazyChecksUnderline.append( std::string( "--clazy-checks=" ).size(), '~' );
    std::string strClangArgsUnderline( std::string( "-p=pathToCompilationDataBase "
                                       "--export-fixes=pathToResultYaml --clazy-checks= " ).size(), ' ' );
    strClangArgsUnderline.append( std::string( "--clang-extra-args=" ).size(), '~' );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData() ==
                 "Warning: Parameter \"clazy-checks\" was "
                 "set but the parameter's value was not set. "
                 "The parameter will be ignored.\n"
                 "-p=pathToCompilationDataBase "
                 "--export-fixes=pathToResultYaml "
                 "--clazy-checks= --clang-extra-args=\n" + strClazyChecksUnderline + "\n"
                 "Warning: Parameter \"clang-extra-args\" was "
                 "set but the parameter's value was not set. "
                 "The parameter will be ignored.\n"
                 "-p=pathToCompilationDataBase "
                 "--export-fixes=pathToResultYaml "
                 "--clazy-checks= --clang-extra-args=\n" + strClangArgsUnderline + "\n" );
}

BOOST_AUTO_TEST_CASE( TestClazyChecksSyntaxError ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
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

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    std::string strClazyChecksUnderLine( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml " ).size(), ' ' );
    strClazyChecksUnderLine.append( std::string( "--clazy-checks" ).size(), '~' );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData() ==
                 "Warning: Parameter \"clazy-checks\" was set "
                 "but the parameter's value was not set. "
                 "The parameter will be ignored.\n"
                 "-p=pathToCompilationDataBase "
                 "--export-fixes=pathToResultYaml "
                 "--clazy-checks\n" + strClazyChecksUnderLine + "\n" );
}

BOOST_AUTO_TEST_CASE( TestClangExtraArgsSyntaxError ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
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

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    std::string strClangArgsUnderline( std::string( "-p=pathToCompilationDataBase "
                                       "--export-fixes=pathToResultYaml " ).size(), ' ' );
    strClangArgsUnderline.append( std::string( "--clang-extra-args" ).size(), '~' );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData() ==
                 "Warning: Parameter \"clang-extra-args\" was set "
                 "but the parameter's value was not set. "
                 "The parameter will be ignored.\n"
                 "-p=pathToCompilationDataBase "
                 "--export-fixes=pathToResultYaml "
                 "--clang-extra-args\n" + strClangArgsUnderline + "\n" );
}

BOOST_AUTO_TEST_CASE( TestAllParamsHaveSyntaxError ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
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

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    std::string strClazyChecksUnderLine( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml " ).size(), ' ' );
    strClazyChecksUnderLine.append( std::string( "--clazy-checks" ).size(), '~' );
    std::string strClangArgsUnderline( std::string( "-p=pathToCompilationDataBase "
                                       "--export-fixes=pathToResultYaml --clazy-checks " ).size(), ' ' );
    strClangArgsUnderline.append( std::string( "--clang-extra-args" ).size(), '~' );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData() ==
                 "Warning: Parameter \"clazy-checks\" was set "
                 "but the parameter's value was not set. "
                 "The parameter will be ignored.\n"
                 "-p=pathToCompilationDataBase "
                 "--export-fixes=pathToResultYaml "
                 "--clazy-checks --clang-extra-args\n" + strClazyChecksUnderLine + "\n"
                 "Warning: Parameter \"clang-extra-args\" was set "
                 "but the parameter's value was not set. "
                 "The parameter will be ignored.\n"
                 "-p=pathToCompilationDataBase "
                 "--export-fixes=pathToResultYaml "
                 "--clazy-checks --clang-extra-args\n" + strClangArgsUnderline + "\n" );
}

BOOST_AUTO_TEST_CASE( TestClazyChecksExist ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
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

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    BOOST_CHECK( stderrCapture.getBufferData().empty() );
}

BOOST_AUTO_TEST_CASE( TestClangExtraArgsExist ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
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

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    BOOST_CHECK( stderrCapture.getBufferData().empty() );
}

BOOST_AUTO_TEST_CASE( TestAllParamsExistAfterEqualSign ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
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

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    BOOST_CHECK( stderrCapture.getBufferData().empty() );
}

BOOST_AUTO_TEST_CASE( TestAllParamsExistAfterSpace ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine
        = { "-p=pathToCompilationDataBase",
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

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    BOOST_CHECK( stderrCapture.getBufferData().empty() );
}

BOOST_AUTO_TEST_CASE( TestOneSublinterValueEmptyAfterEqualSign ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--sub-linter=" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == true );
    std::string strSublinterUnderline( std::string( "-p=pathToCompilationDataBase "
                                       "--export-fixes=pathToResultYaml " ).size(), ' ' );
    strSublinterUnderline.append( std::string( "--sub-linter=" ).size(), '~' );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData() == "Error: Value of parameter \"sub-linter\" "
                                                   "must follow after the equal sign\n"
                                                   "-p=pathToCompilationDataBase "
                                                   "--export-fixes=pathToResultYaml "
                                                   "--sub-linter=\n" + strSublinterUnderline + "\n" );
}

BOOST_AUTO_TEST_CASE( TestFirstSubLinterIncorrectSecondValueEmptyAfterEqualSign ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--sub-linter=IncorrectName_1", "--sub-linter=" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == true );
    std::string strSublinterUnderline_1( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml --sub-linter=" ).size(), ' ' );
    strSublinterUnderline_1.append( std::string( "IncorrectName_1" ).size(), '~' );
    std::string strSublinterUnderline_2( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml --sub-linter=IncorrectName_1 " )
                                         .size(), ' ' );
    strSublinterUnderline_2.append( std::string( "--sub-linter=" ).size(), '~' );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData()
                 == "Error: Unknown linter name: \"IncorrectName_1\"\n"
                    "-p=pathToCompilationDataBase "
                    "--export-fixes=pathToResultYaml "
                    "--sub-linter=IncorrectName_1 --sub-linter=\n"
                    + strSublinterUnderline_1 + "\n"
                    "Error: Value of parameter \"sub-linter\" "
                    "must follow after the equal sign\n"
                    "-p=pathToCompilationDataBase "
                    "--export-fixes=pathToResultYaml "
                    "--sub-linter=IncorrectName_1 --sub-linter=\n"
                    + strSublinterUnderline_2 + "\n" );
}

BOOST_AUTO_TEST_CASE( TestFirstSubLinterValueEmptyAfterEqualSignSecondIncorrect ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--sub-linter=", "--sub-linter=IncorrectName_1" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == true );
    std::string strSublinterUnderline_1( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml " ).size(), ' ' );
    strSublinterUnderline_1.append( std::string( "--sub-linter=" ).size(), '~' );
    std::string strSublinterUnderline_2( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml --sub-linter= --sub-linter=" )
                                         .size(), ' ' );
    strSublinterUnderline_2.append( std::string( "IncorrectName_1" ).size(), '~' );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData()
             == "Error: Value of parameter \"sub-linter\" "
                "must follow after the equal sign\n"
                "-p=pathToCompilationDataBase "
                "--export-fixes=pathToResultYaml "
                "--sub-linter= --sub-linter=IncorrectName_1\n"
                + strSublinterUnderline_1 + "\n"
                 "Error: Unknown linter name: \"IncorrectName_1\"\n"
                "-p=pathToCompilationDataBase "
                "--export-fixes=pathToResultYaml "
                "--sub-linter= --sub-linter=IncorrectName_1\n"
                + strSublinterUnderline_2 + "\n" );
}

BOOST_AUTO_TEST_CASE( TestTwoSublinterValuesEmptyAfterEqualSign ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--sub-linter=", "--sub-linter=" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == true );
    std::string strSublinterUnderline_1( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml " ).size(), ' ' );
    strSublinterUnderline_1.append( std::string( "--sub-linter=" ).size(), '~' );
    std::string strSublinterUnderline_2( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml --sub-linter= " ).size(), ' ' );
    strSublinterUnderline_2.append( std::string( "--sub-linter=" ).size(), '~' );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData()
                 == "Error: Value of parameter \"sub-linter\" "
                    "must follow after the equal sign\n"
                    "-p=pathToCompilationDataBase "
                    "--export-fixes=pathToResultYaml "
                    "--sub-linter= --sub-linter=\n"
                    + strSublinterUnderline_1 + "\n"
                    "Error: Value of parameter \"sub-linter\" "
                    "must follow after the equal sign\n"
                     "-p=pathToCompilationDataBase "
                    "--export-fixes=pathToResultYaml "
                    "--sub-linter= --sub-linter=\n"
                    + strSublinterUnderline_2 + "\n" );
}

BOOST_AUTO_TEST_CASE( TestOneSublinterHaveSyntaxError ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--sub-linter" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == true );
    std::string strSublinterUnderline( std::string( "-p=pathToCompilationDataBase "
                                       "--export-fixes=pathToResultYaml --sub-linter" ).size(), ' ' );
    strSublinterUnderline.append( "~~~" );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData() ==
                 "Error: Unknown linter name: \"\"\n"
                 "-p=pathToCompilationDataBase --export-fixes=pathToResultYaml "
                 "--sub-linter\n" + strSublinterUnderline + "\n" );
}

BOOST_AUTO_TEST_CASE( TestFirstSubLinterIncorrectSecondHasEmptyValueAfterSpace ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--sub-linter", "IncorrectName_1", "--sub-linter" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == true );
    std::string strSublinterUnderline_1( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml --sub-linter " ).size(), ' ' );
    strSublinterUnderline_1.append( std::string( "IncorrectName_1" ).size(), '~' );
    std::string strSublinterUnderline_2( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml --sub-linter IncorrectName_1 --sub-linter" )
                                         .size(), ' ' );
    strSublinterUnderline_2.append( 3, '~' );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData()
     == "Error: Unknown linter name: \"IncorrectName_1\"\n"
        "-p=pathToCompilationDataBase "
        "--export-fixes=pathToResultYaml "
        "--sub-linter IncorrectName_1 --sub-linter\n"
        + strSublinterUnderline_1 + "\n"
        "Error: Unknown linter name: \"\"\n"
        "-p=pathToCompilationDataBase "
        "--export-fixes=pathToResultYaml "
        "--sub-linter IncorrectName_1 --sub-linter\n"
        + strSublinterUnderline_2 + "\n" );
}

BOOST_AUTO_TEST_CASE( TestFirstSubLinterHasEmptyValueAfterSpaceSecondIncorrect ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--sub-linter", "--sub-linter", "IncorrectName_1" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == true );
    std::string strSublinterUnderline_1( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml --sub-linter " ).size(), ' ' );
    strSublinterUnderline_1.append( std::string( "--sub-linter" ).size(), '~' );
    std::string strSublinterUnderline_2( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml --sub-linter --sub-linter " )
                                         .size(), ' ' );
    strSublinterUnderline_2.append( std::string( "IncorrectName_1" ).size(), '~' );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData()
             == "Error: Unknown linter name: \"--sub-linter\"\n"
                "-p=pathToCompilationDataBase "
                "--export-fixes=pathToResultYaml "
                "--sub-linter --sub-linter IncorrectName_1\n"
                + strSublinterUnderline_1 + "\n"
                 "Error: Unknown linter name: \"IncorrectName_1\"\n"
                "-p=pathToCompilationDataBase "
                "--export-fixes=pathToResultYaml "
                "--sub-linter --sub-linter IncorrectName_1\n"
                + strSublinterUnderline_2 + "\n" );
}

// TODO:3
BOOST_AUTO_TEST_CASE( TestSeveralSubLinterHasEmptyValueAfterSpace ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine
        = { "-p=pathToCompilationDataBase",
            "--export-fixes=pathToResultYaml",
            "--sub-linter", "--sub-linter", "--sub-linter" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    std::string strSublinterUnderline_1( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml --sub-linter " ).size(), ' ' );
    strSublinterUnderline_1.append( std::string( "--sub-linter" ).size(), '~' );
    std::string strSublinterUnderline_2( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml --sub-linter "
                                         "--sub-linter " ).size(), ' ' );
    strSublinterUnderline_2.append( std::string( "--sub-linter" ).size(), '~' );
    std::string strSublinterUnderline_3( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml --sub-linter "
                                         "--sub-linter --sub-linter" ).size(), ' ' );
    strSublinterUnderline_3.append( 3, '~' );
    std::cout << stderrCapture.getBufferData() << std::endl;
    BOOST_CHECK( stderrCapture.getBufferData()
             == "Error: Unknown linter name: \"--sub-linter\"\n"
        "-p=pathToCompilationDataBase "
        "--export-fixes=pathToResultYaml "
        "--sub-linter --sub-linter --sub-linter\n"
        + strSublinterUnderline_1 + "\n"
        "Error: Unknown linter name: \"--sub-linter\"\n"
        "-p=pathToCompilationDataBase "
        "--export-fixes=pathToResultYaml "
        "--sub-linter --sub-linter --sub-linter\n"
        + strSublinterUnderline_2 + "\n"
        "Error: Unknown linter name: \"\"\n"
        "-p=pathToCompilationDataBase "
        "--export-fixes=pathToResultYaml "
        "--sub-linter --sub-linter --sub-linter\n"
        + strSublinterUnderline_3 + "\n" );
}

BOOST_AUTO_TEST_CASE( TestTwoSublinterHaveSyntaxError ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--sub-linter", "--sub-linter" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == true );
    std::string strSublinterUnderline_1( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml --sub-linter " ).size(), ' ' );
    strSublinterUnderline_1.append( std::string( "--sub-linter" ).size(), '~' );
    std::string strSublinterUnderline_2( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml --sub-linter --sub-linter" )
                                         .size(), ' ' );
    strSublinterUnderline_2.append( "~~~" );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData()
                 == "Error: Unknown linter name: \"--sub-linter\"\n"
                    "-p=pathToCompilationDataBase "
                    "--export-fixes=pathToResultYaml "
                    "--sub-linter --sub-linter\n"
                    + strSublinterUnderline_1 + "\n"
                    "Error: Unknown linter name: \"\"\n"
                    "-p=pathToCompilationDataBase "
                    "--export-fixes=pathToResultYaml "
                    "--sub-linter --sub-linter\n"
                    + strSublinterUnderline_2 + "\n" );
}

BOOST_AUTO_TEST_CASE( TestOneSublinterIncorrectName ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--sub-linter=IncorrectName_1" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == true );
    std::string strSublinterUnderline_1( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml --sub-linter=" ).size(), ' ' );
    strSublinterUnderline_1.append( std::string( "IncorrectName_1" ).size(), '~' );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData() ==
                 "Error: Unknown linter name: \"IncorrectName_1\"\n"
                 "-p=pathToCompilationDataBase --export-fixes=pathToResultYaml "
                 "--sub-linter=IncorrectName_1\n" + strSublinterUnderline_1 + "\n" );
}

BOOST_AUTO_TEST_CASE( TestTwoSublinterIncorrectName ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--sub-linter=IncorrectName_1",
                                              "--sub-linter=IncorrectName_2" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == true );
    std::string strSublinterUnderline_1( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml --sub-linter=" ).size(), ' ' );
    strSublinterUnderline_1.append( std::string( "IncorrectName_1" ).size(), '~' );
    std::string strSublinterUnderline_2( std::string( "-p=pathToCompilationDataBase "
                                         "--export-fixes=pathToResultYaml "
                                         "--sub-linter=IncorrectName_1 --sub-linter=" ).size(), ' ' );
    strSublinterUnderline_2.append( std::string( "IncorrectName_2" ).size(), '~' );
    std::cout << stderrCapture.getBufferData();
    BOOST_CHECK( stderrCapture.getBufferData()
                 == "Error: Unknown linter name: \"IncorrectName_1\"\n"
                    "-p=pathToCompilationDataBase --export-fixes=pathToResultYaml "
                    "--sub-linter=IncorrectName_1 --sub-linter=IncorrectName_2\n"
                    + strSublinterUnderline_1 + "\n"
                    "Error: Unknown linter name: \"IncorrectName_2\"\n"
                    "-p=pathToCompilationDataBase --export-fixes=pathToResultYaml "
                    "--sub-linter=IncorrectName_1 --sub-linter=IncorrectName_2\n"
                    + strSublinterUnderline_2 + "\n" );
}

BOOST_AUTO_TEST_CASE( TestSublinterIsClangTidy ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--sub-linter=clang-tidy" };
    const std::array < std::string, 4 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    BOOST_CHECK( stderrCapture.getBufferData().empty() );
}

BOOST_AUTO_TEST_CASE( TestSublinterIsClazy ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--sub-linter=clazy" };
    const std::array < std::string, 4 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    BOOST_CHECK( stderrCapture.getBufferData().empty() );
}

BOOST_AUTO_TEST_CASE( TestSublintersAreClangTidyAndClazyAfterEqualSign ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "--sub-linter=clang-tidy",
                                              "--sub-linter=clazy" };
    const std::array < std::string, 7 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );
    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    BOOST_CHECK( stderrCapture.getBufferData().empty() );
}

BOOST_AUTO_TEST_CASE( TestSublintersAreClangTidyAndClazyAfterSpace ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLine = { "-p=pathToCompilationDataBase",
                                              "--export-fixes=pathToResultYaml",
                                              "-sub-linter", "clang-tidy",
                                              "-sub-linter", "clazy" };
    const std::array < std::string, 7 > result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase\\diagnosticsClazy.yaml" };

    const LintCombine::CommandLinePreparer commandLinePreparer( commandLine, "ReSharper" );

    compareVectors( commandLine, result );
    BOOST_CHECK( commandLinePreparer.getIsErrorWhilePrepareOccur() == false );
    BOOST_CHECK( stderrCapture.getBufferData().empty() );
}

BOOST_AUTO_TEST_SUITE_END()
