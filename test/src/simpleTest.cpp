#define BOOST_TEST_MODULE CppLintCombineTest

#include "../../src/LinterCombine.h"
#include "../../src/LinterBase.h"
#include "../../src/IdeTraitsFactory.h"

#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <filesystem>
#include <memory>
#include <optional>

class StreamCapture {
public:
    explicit StreamCapture( std::ostream & stream ) : m_stream( &stream ) {
        m_old = stream.flush().rdbuf( m_buffer.rdbuf() );
    }

    ~StreamCapture() {
        m_buffer.flush();
        m_stream->rdbuf( m_old );
    }

    std::string getBufferData() const {
        return m_buffer.str();
    }

private:
    std::ostream * m_stream;
    std::ostringstream m_buffer;
    std::streambuf * m_old;
};

namespace LintCombine {
    struct MockLinterWrapper final : LinterBase {
        MockLinterWrapper( const stringVector & commandLine,
                           LinterFactoryBase::Services & service ) : LinterBase( service ) {
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

        std::shared_ptr< LinterItf >
            createLinter( const stringVector & cmdLine ) override {
            if( cmdLine[0] == "MockLinterWrapper" ) {
                return std::make_shared< MockLinterWrapper >( cmdLine, getServices() );
            }
            return nullptr;
        }

    private:
        MocksLinterFactory() = default;
    };
}

void compareDiagnostics( const LintCombine::Diagnostic & lhs,
                         const LintCombine::Diagnostic & rhs ) {
    BOOST_CHECK( lhs.level == rhs.level );
    BOOST_CHECK( lhs.origin == rhs.origin );
    BOOST_CHECK( lhs.firstPos == rhs.firstPos );
    BOOST_CHECK( lhs.lastPos == rhs.lastPos );
    BOOST_CHECK( lhs.text == rhs.text );
}

void recoverYamlFiles() {
    std::filesystem::remove( CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" );
    std::filesystem::copy_file( CURRENT_SOURCE_DIR "yamlFiles/linterFile_1_save.yaml",
                                CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" );
    std::filesystem::remove( CURRENT_SOURCE_DIR "yamlFiles/linterFile_2.yaml" );
    std::filesystem::copy_file( CURRENT_SOURCE_DIR "yamlFiles/linterFile_2_save.yaml",
                                CURRENT_SOURCE_DIR "yamlFiles/linterFile_2.yaml" );
}

BOOST_AUTO_TEST_SUITE( TestLinterCombineConstructor )

/*
 * Tests names abbreviations:
 * L<n> means: <n>-th linter
 * DNE means: do/does not exists
 * E means: Exists
 * VEAES means: params value empty after equal sign
 * VEASP means: params value empty after space
 * VI means: params value incorrect
*/

// LCC means LinterCombineConstructor
struct LCCTestCase {

    struct LinterData {
        LinterData( const std::string & nameVal,
                    const std::string & optionsVal,
                    const std::string & yamlPathVal )
            : name( nameVal ), options( optionsVal ),
            yamlPath( yamlPathVal ) {}
        std::string name;
        std::string options;
        std::string yamlPath;
    };

    struct Output {
        Output( const std::vector< LintCombine::Diagnostic > & diagnosticsVal,
                const std::vector< LinterData > & linterDataVal,
                const bool exceptionOccurVal )
            : diagnostics( diagnosticsVal ),
            linterData( linterDataVal ),
            exceptionOccur( exceptionOccurVal ) {}
        std::vector< LintCombine::Diagnostic > diagnostics;
        std::vector< LinterData > linterData;
        bool exceptionOccur = false;
    };

    struct Input {
        Input( const LintCombine::stringVector & cmdLineVal )
            : cmdLine( cmdLineVal ) {}
        LintCombine::stringVector cmdLine;
    };

    LCCTestCase( const Input & inputVal,
                 const Output & outputVal )
        : input( inputVal ), output( outputVal ) {}

    Input input;
    Output output;
};

std::ostream & operator<<( std::ostream & os, LCCTestCase ) {
    return os;
}

namespace TestLCC::EmptyCmdLine {
    const LCCTestCase::Input input{ LintCombine::stringVector() };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
        "Command Line is empty", "Combine", 1, 0 ) };
    const std::vector< LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::NotOneLintersSet {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--param_1=value_1", "--param_2=value_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
        "No one linter parsed", "Combine", 1, 0 ) };
    const std::vector< LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::L1DNE {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter=NotExistentLinter" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
        "Unknown linter name: \"NotExistentLinter\"", "Combine", 1, 0 ) };
    const std::vector< LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::L1DNE_L2E {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter=NotExistentLinter",
                                   "--sub-linter=clazy" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
        "Unknown linter name: \"NotExistentLinter\"", "Combine", 1, 0 ) };
    const std::vector< LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::L1DNE_L2DNE {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter=NotExistentLinter_1",
                                   "--sub-linter=NotExistentLinter_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
        "Unknown linter name: \"NotExistentLinter_1\"", "Combine", 1, 0 )
    };
    const std::vector< LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::OneLintersVEAES {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the argument for option '--sub-linter' should follow "
            "immediately after the equal sign", "Combine", 1, 0 )
    };
    const std::vector< LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::OneLintersVEASP {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the required argument for option '--sub-linter' "
            "is missing", "Combine", 1, 0 )
    };
    const std::vector< LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::TwoLintersVEAES {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter=", "--sub-linter=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the argument for option '--sub-linter' should follow "
            "immediately after the equal sign", "Combine", 1, 0 )
    };
    const std::vector< LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::TwoLintersVEASP {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter", "--sub-linter" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "No one linter parsed", "Combine", 1, 0 )
    };
    const std::vector< LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::GeneralYamlPathVEAES {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=", "--sub-linter=clazy" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the argument for option '--result-yaml' should follow "
            "immediately after the equal sign", "Combine", 1, 0 )
    };
    const std::vector< LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::GeneralYamlPathVEASP {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml", "--sub-linter=clazy",
            "--export-fixes=" CURRENT_BINARY_DIR "mockL" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "General YAML-file \"--sub-linter=clazy\" "
            "is not creatable", "Combine", 1, 0 )
    };
    const std::vector< LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::GeneralYamlPathVI {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=\\\\", "--sub-linter=clazy",
            "--export-fixes=" CURRENT_BINARY_DIR "mockL" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "General YAML-file \"\\\\\" is not creatable", "Combine", 1, 0 )
    };
    const std::vector< LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::LinterYamlPathVEAES {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter=clazy", "--export-fixes=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the argument for option '--export-fixes' should "
            "follow immediately after the equal sign", "Combine", 1, 0 )
    };
    const std::vector< LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::LinterYamlPathVEASP {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=" CURRENT_BINARY_DIR
            "mockG", "--sub-linter=clazy", "--export-fixes" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the required argument for option '--export-fixes' "
            "is missing", "clazy-standalone", 1, 0 )
    };
    const std::vector< LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::LinterYamlPathVI {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=" CURRENT_BINARY_DIR
            "mockG", "--sub-linter=clazy", "--export-fixes=\\\\" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Linter's YAML-file \"\\\\\" is not creatable", "clazy-standalone", 1, 0 )
    };
    const std::vector< LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::ClazyExists {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=" CURRENT_BINARY_DIR "mockR",
            "--sub-linter=clazy", "--export-fixes=" CURRENT_BINARY_DIR "mockL" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const std::vector< LCCTestCase::LinterData > linterData{
        LCCTestCase::LinterData( "clazy-standalone", std::string(),
        CURRENT_BINARY_DIR "mockL" )
    };
    const LCCTestCase::Output output{ diagnostics, linterData, false };
}

namespace TestLCC::ClangTidyExists {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=" CURRENT_BINARY_DIR "mockR",
            "--sub-linter=clang-tidy", "--export-fixes=" CURRENT_BINARY_DIR "mockL" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const std::vector< LCCTestCase::LinterData > linterData{
        LCCTestCase::LinterData( "clang-tidy", std::string(),
        CURRENT_BINARY_DIR "mockL" )
    };
    const LCCTestCase::Output output{ diagnostics, linterData, false };
}

namespace TestLCC::ClazyEWithOptions {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=" CURRENT_BINARY_DIR "mockR",
            "--sub-linter=clazy", "--export-fixes=" CURRENT_BINARY_DIR "mockL",
            "CLParam_1", "CLParam_2"} };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const std::vector< LCCTestCase::LinterData > linterData{
        LCCTestCase::LinterData( "clazy-standalone", "CLParam_1 CLParam_2 ",
        CURRENT_BINARY_DIR "mockL" )
    };
    const LCCTestCase::Output output{ diagnostics, linterData, false };
}

namespace TestLCC::ClangTidyAndClazyE {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=" CURRENT_BINARY_DIR "mockR",
            "--sub-linter=clang-tidy", "--export-fixes=" CURRENT_BINARY_DIR "mockL",
            "--sub-linter=clazy", "--export-fixes=" CURRENT_BINARY_DIR "mockL" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const std::vector< LCCTestCase::LinterData > linterData{
        LCCTestCase::LinterData( "clang-tidy", std::string(),
        CURRENT_BINARY_DIR "mockL" ),
        LCCTestCase::LinterData( "clazy-standalone", std::string(),
        CURRENT_BINARY_DIR "mockL" )
    };
    const LCCTestCase::Output output{ diagnostics, linterData, false };
}

namespace TestLCC::ClangTidyAndClazyEWithOptions {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=" CURRENT_BINARY_DIR "mockR",
            "--sub-linter=clang-tidy", "--export-fixes=" CURRENT_BINARY_DIR "mockL",
            "CTParam_1", "CTParam_2",
            "--sub-linter=clazy", "--export-fixes=" CURRENT_BINARY_DIR "mockL",
            "CLParam_1", "CLParam_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const std::vector< LCCTestCase::LinterData > linterData{
        LCCTestCase::LinterData( "clang-tidy", "CTParam_1 CTParam_2 ",
        CURRENT_BINARY_DIR "mockL" ),
        LCCTestCase::LinterData( "clazy-standalone", "CLParam_1 CLParam_2 ",
        CURRENT_BINARY_DIR "mockL" )
    };
    const LCCTestCase::Output output{ diagnostics, linterData, false };
}

const std::vector< LCCTestCase > LCCTestCaseData = {
    /*0 */    { TestLCC::EmptyCmdLine::input, TestLCC::EmptyCmdLine::output },
    /*1 */    { TestLCC::NotOneLintersSet::input, TestLCC::NotOneLintersSet::output },
    /*2 */    { TestLCC::L1DNE::input, TestLCC::L1DNE::output },
    /*3 */    { TestLCC::L1DNE_L2E::input, TestLCC::L1DNE_L2E::output },
    /*4 */    { TestLCC::L1DNE_L2DNE::input, TestLCC::L1DNE_L2DNE::output },
    /*5 */    { TestLCC::OneLintersVEAES::input, TestLCC::OneLintersVEAES::output },
    /*6 */    { TestLCC::OneLintersVEASP::input, TestLCC::OneLintersVEASP::output },
    /*7 */    { TestLCC::TwoLintersVEAES::input, TestLCC::TwoLintersVEAES::output },
    /*8 */    { TestLCC::TwoLintersVEASP::input, TestLCC::TwoLintersVEASP::output },
    /*9 */    { TestLCC::GeneralYamlPathVEAES::input, TestLCC::GeneralYamlPathVEAES::output },
    /*10*/    { TestLCC::GeneralYamlPathVEASP::input, TestLCC::GeneralYamlPathVEASP::output },
    /*11*/    { TestLCC::GeneralYamlPathVI::input, TestLCC::GeneralYamlPathVI::output },
    /*12*/    { TestLCC::LinterYamlPathVEAES::input, TestLCC::LinterYamlPathVEAES::output },
    /*13*/    { TestLCC::LinterYamlPathVEASP::input, TestLCC::LinterYamlPathVEASP::output },
    /*14*/    { TestLCC::LinterYamlPathVI::input, TestLCC::LinterYamlPathVI::output },
    /*15*/    { TestLCC::ClazyExists::input, TestLCC::ClazyExists::output },
    /*16*/    { TestLCC::ClangTidyExists::input, TestLCC::ClangTidyExists::output },
    /*17*/    { TestLCC::ClazyEWithOptions::input, TestLCC::ClazyEWithOptions::output },
    /*18*/    { TestLCC::ClangTidyAndClazyE::input, TestLCC::ClangTidyAndClazyE::output },
    /*19*/    { TestLCC::ClangTidyAndClazyEWithOptions::input, TestLCC::ClangTidyAndClazyEWithOptions::output }
};

BOOST_DATA_TEST_CASE( TestLinterCombineConstructor, LCCTestCaseData, sample ) {
    const auto & correctResult = static_cast< LCCTestCase::Output >( sample.output );
    if( correctResult.exceptionOccur ) {
        try {
            LintCombine::LinterCombine combine( sample.input.cmdLine );
        }
        catch( const LintCombine::Exception & ex ) {
            BOOST_REQUIRE( ex.diagnostics().size() ==
                           correctResult.diagnostics.size() );
            for( size_t i = 0; i < correctResult.diagnostics.size(); ++i ) {
                compareDiagnostics( ex.diagnostics()[i],
                                    correctResult.diagnostics[i] );
            }
        }
    }
    else {
        LintCombine::LinterCombine combine( sample.input.cmdLine );
        BOOST_CHECK( combine.numLinters() == correctResult.linterData.size() );
        for( size_t i = 0; i < combine.numLinters(); ++i ) {
            const auto & linter =
                std::dynamic_pointer_cast < LintCombine::LinterBase > (
                combine.linterAt( i ) );
            BOOST_CHECK( correctResult.linterData[i].name == linter->getName() );
            BOOST_CHECK( correctResult.linterData[i].options == linter->getOptions() );
            BOOST_CHECK( correctResult.linterData[i].yamlPath == linter->getYamlPath() );
        }
        const auto & combineDiagnostics = combine.diagnostics();
        const auto & correctResultDiagnostics = correctResult.diagnostics;
        BOOST_REQUIRE( combineDiagnostics.size() == correctResultDiagnostics.size() );
        for( size_t i = 0; i < correctResultDiagnostics.size(); ++i ) {
            compareDiagnostics( combineDiagnostics[i], correctResultDiagnostics[i] );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestCallAndWaitLinter )

/*
 * Tests names abbreviations:
 * L<n> means: <n>-th linter
 * WS means: write to streams (stdout, stderr)
 * WF means: write to file
 * WSF means: write to file and to streams (stdout, stderr)
 * EETC means: ends earlier than combine
 * R<n> means: return <n>
*/

// CWL means CallAndWaitLinter
struct CWLTestCase {
    struct FileData {
        FileData( const std::string & filenameVal,
                  const LintCombine::stringVector & fileDataVal )
            : filename( filenameVal ), fileData( fileDataVal ) {}
        std::string filename;
        LintCombine::stringVector fileData;
    };

    struct Output {
        Output( const std::vector< LintCombine::Diagnostic > & diagnosticsVal,
                const LintCombine::stringVector & stdoutDataVal,
                const LintCombine::stringVector & stderrDataVal,
                const std::vector< FileData > & filesWithContentVal,
                const int returnCodeVal )
            : diagnostics( diagnosticsVal ), stdoutData( stdoutDataVal ),
            stderrData( stderrDataVal ), filesWithContent( filesWithContentVal ),
            returnCode( returnCodeVal ) {}
        std::vector< LintCombine::Diagnostic > diagnostics;
        LintCombine::stringVector stdoutData;
        LintCombine::stringVector stderrData;
        std::vector<FileData> filesWithContent;
        int returnCode;
    };

    struct Input {
        Input( const LintCombine::stringVector & cmdLineVal,
               const LintCombine::stringVector & fileNamesForLinterEndsEarlyTestVal
               = LintCombine::stringVector() )
            : cmdLine( cmdLineVal ),
            fileNamesForLinterEndsEarlyTest( fileNamesForLinterEndsEarlyTestVal ) {}
        LintCombine::stringVector cmdLine;
        // files to check that linter ends early than combine
        LintCombine::stringVector fileNamesForLinterEndsEarlyTest;
    };

    CWLTestCase( const Input & inputVal,
                 const Output & outputVal )
        : input( inputVal ), output( outputVal ) {}

    Input input;
    Output output;
};

std::ostream & operator<<( std::ostream & os, CWLTestCase ) {
    return os;
}

std::string getRunnerName( const std::string & shellName ) {
    if constexpr( BOOST_OS_WINDOWS ) {
        if( shellName == "cmd" ) { return std::string(); }
        if( shellName == "bash" ) { return "sh.exe "; }
    }
    if constexpr( BOOST_OS_LINUX ) { return "sh "; }
    return std::string();
}

std::string getScriptExtension() {
    if constexpr( BOOST_OS_WINDOWS ) { return ".bat"; }
    if constexpr( BOOST_OS_LINUX ) { return ".sh"; }
    return std::string();
}

namespace TestCWL::L1Terminate {
    const CWLTestCase::Input input{
        LintCombine::stringVector { "--result-yaml=" CURRENT_BINARY_DIR "mockG",
            "--sub-linter=MockLinterWrapper", getRunnerName( "bash" ) + CURRENT_SOURCE_DIR
            "mockPrograms/mockWriteToStdoutAndTerminate.sh stdoutLinter_1" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "All linters failed while running", "Combine", 1, 0 )
    };
    const LintCombine::stringVector stdoutData{ "stdoutLinter_1" };
    const LintCombine::stringVector stderrData;
    const std::vector< CWLTestCase::FileData > filesWithContent;
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, /*returnCode*/3 };
}

namespace TestCWL::L1Terminate_L2WSFR0 {
    const CWLTestCase::Input input{
        LintCombine::stringVector { "--result-yaml=" CURRENT_BINARY_DIR "mockG",
            "--sub-linter=MockLinterWrapper", getRunnerName( "bash" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStdoutAndTerminate.sh "
            "stdoutLinter_1", "--sub-linter=MockLinterWrapper",
            getRunnerName( "cmd" ) + CURRENT_SOURCE_DIR
            "mockPrograms/mockWriteToStreamsAndToFile" +
            getScriptExtension() + " 0 " CURRENT_BINARY_DIR
            "MockFile_2 fileLinter_2 " "stdoutLinter_2 stderrLinter_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Warning,
            "Some linters failed while running", "Combine", 1, 0 )
    };
    const LintCombine::stringVector stdoutData{ "stdoutLinter_1", "stdoutLinter_2" };
    const LintCombine::stringVector stderrData{ "stderrLinter_2" };
    const std::vector< CWLTestCase::FileData > filesWithContent{
        CWLTestCase::FileData{ "MockFile_2", LintCombine::stringVector{"fileLinter_2"} }
    };
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, /*returnCode*/2 };
}

namespace TestCWL::L1Terminate_L2Terminate {
    const CWLTestCase::Input input{
        LintCombine::stringVector { "--result-yaml=" CURRENT_BINARY_DIR "mockG",
            "--sub-linter=MockLinterWrapper", getRunnerName( "bash" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStdoutAndTerminate.sh "
            "stdoutLinter_1", "--sub-linter=MockLinterWrapper",
            getRunnerName( "bash" ) + CURRENT_SOURCE_DIR
            "mockPrograms/mockWriteToStdoutAndTerminate.sh stdoutLinter_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "All linters failed while running", "Combine", 1, 0 )
    };
    const LintCombine::stringVector stdoutData{ "stdoutLinter_1", "stdoutLinter_2" };
    const LintCombine::stringVector stderrData;
    const std::vector< CWLTestCase::FileData > filesWithContent;
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, /*returnCode*/3 };
}

namespace TestCWL::L1WSR1 {
    const CWLTestCase::Input input{
        LintCombine::stringVector { "--result-yaml=" CURRENT_BINARY_DIR "mockG",
            "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreams" +
            getScriptExtension() + " 1 stdoutLinter_1 stderrLinter_1" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "All linters failed while running", "Combine", 1, 0 )
    };
    const LintCombine::stringVector stdoutData{ "stdoutLinter_1" };
    const LintCombine::stringVector stderrData{ "stderrLinter_1" };
    const std::vector< CWLTestCase::FileData > filesWithContent;
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, /*returnCode*/3 };
}

namespace TestCWL::L1WSR1_L2WSFR0 {
    const CWLTestCase::Input input{
        LintCombine::stringVector { "--result-yaml=" CURRENT_BINARY_DIR "mockG",
            "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreams" +
            getScriptExtension() + " 1 stdoutLinter_1 stderrLinter_1",
            "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreamsAndToFile" +
            getScriptExtension() + " 0 " CURRENT_BINARY_DIR "MockFile_2 "
            "fileLinter_2 stdoutLinter_2 stderrLinter_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Warning,
            "Some linters failed while running", "Combine", 1, 0 )
    };
    const LintCombine::stringVector stdoutData{ "stdoutLinter_1", "stdoutLinter_2" };
    const LintCombine::stringVector stderrData{ "stderrLinter_1", "stderrLinter_2" };
    const std::vector< CWLTestCase::FileData > filesWithContent{
        CWLTestCase::FileData{ "MockFile_2", LintCombine::stringVector{"fileLinter_2"} }
    };
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, /*returnCode*/2 };
}

namespace TestCWL::L1R1_L2R1 {
    const CWLTestCase::Input input{
        LintCombine::stringVector { "--result-yaml=" CURRENT_BINARY_DIR "mockG",
            "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreams" +
            getScriptExtension() + " 1 stdoutLinter_1 stderrLinter_1",
            "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreams" +
            getScriptExtension() + " 2 stdoutLinter_2 stderrLinter_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "All linters failed while running", "Combine", 1, 0 )
    };
    const LintCombine::stringVector stdoutData{ "stdoutLinter_1", "stdoutLinter_2" };
    const LintCombine::stringVector stderrData{ "stderrLinter_1", "stderrLinter_2" };
    const std::vector< CWLTestCase::FileData > filesWithContent;
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, /*returnCode*/3 };
}

namespace TestCWL::L1WSR0 {
    const CWLTestCase::Input input{
        LintCombine::stringVector { "--result-yaml=" CURRENT_BINARY_DIR "mockG",
            "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreams" +
            getScriptExtension() + " 0 stdoutLinter_1 stderrLinter_1" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const LintCombine::stringVector stdoutData{ "stdoutLinter_1" };
    const LintCombine::stringVector stderrData{ "stderrLinter_1" };
    const std::vector< CWLTestCase::FileData > filesWithContent;
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, /*returnCode*/0 };
}

namespace TestCWL::L1WFR0 {
    const CWLTestCase::Input input{
        LintCombine::stringVector { "--result-yaml=" CURRENT_BINARY_DIR "mockG",
            "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToFile" +
            getScriptExtension() + " 0 MockFile_1 fileLinter_1" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const LintCombine::stringVector stdoutData;
    const LintCombine::stringVector stderrData;
    const std::vector< CWLTestCase::FileData > filesWithContent{
        CWLTestCase::FileData{ "MockFile_1", LintCombine::stringVector{"fileLinter_1"} }
    };
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, /*returnCode*/0 };
}

namespace TestCWL::L1WSFR0 {
    const CWLTestCase::Input input{
        LintCombine::stringVector { "--result-yaml=" CURRENT_BINARY_DIR "mockG",
            "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreamsAndToFile" +
            getScriptExtension() + " 0 MockFile_1 fileLinter_1 "
            "stdoutLinter_1 stderrLinter_1" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const LintCombine::stringVector stdoutData{ "stdoutLinter_1" };
    const LintCombine::stringVector stderrData{ "stderrLinter_1" };
    const std::vector< CWLTestCase::FileData > filesWithContent{
        CWLTestCase::FileData{ "MockFile_1", LintCombine::stringVector{"fileLinter_1"} }
    };
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, /*returnCode*/0 };
}

namespace TestCWL::L1WSR0_L2WSR0 {
    const CWLTestCase::Input input{
        LintCombine::stringVector { "--result-yaml=" CURRENT_BINARY_DIR "mockG",
            "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreams" +
            getScriptExtension() + " 0 stdoutLinter_1 stderrLinter_1",
            "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreams" +
            getScriptExtension() + " 0 stdoutLinter_2 stderrLinter_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const LintCombine::stringVector stdoutData{ "stdoutLinter_1", "stdoutLinter_2" };
    const LintCombine::stringVector stderrData{ "stderrLinter_1", "stderrLinter_2" };
    const std::vector< CWLTestCase::FileData > filesWithContent;
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, /*returnCode*/0 };
}

namespace TestCWL::L1WFR0_L2WFR0 {
    const CWLTestCase::Input input{
        LintCombine::stringVector { "--result-yaml=" CURRENT_BINARY_DIR "mockG",
            "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToFile" +
            getScriptExtension() + " 0 MockFile_1 fileLinter_1",
            "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToFile" +
            getScriptExtension() + " 0 MockFile_2 fileLinter_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const LintCombine::stringVector stdoutData;
    const LintCombine::stringVector stderrData;
    const std::vector< CWLTestCase::FileData > filesWithContent{
        CWLTestCase::FileData{ "MockFile_1", LintCombine::stringVector{"fileLinter_1"} },
        CWLTestCase::FileData{ "MockFile_2", LintCombine::stringVector{"fileLinter_2"} }
    };
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, /*returnCode*/0 };
}

namespace TestCWL::L1WSFR0_L2WSFR0 {
    const CWLTestCase::Input input{
        LintCombine::stringVector { "--result-yaml=" CURRENT_BINARY_DIR "mockG",
            "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreamsAndToFile" +
            getScriptExtension() + " 0 MockFile_1 fileLinter_1 "
            "stdoutLinter_1 stderrLinter_1",
            "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreamsAndToFile" +
            getScriptExtension() + " 0 MockFile_2 fileLinter_2 "
            "stdoutLinter_2 stderrLinter_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const LintCombine::stringVector stdoutData{ "stdoutLinter_1", "stdoutLinter_2" };
    const LintCombine::stringVector stderrData{ "stderrLinter_1", "stderrLinter_2" };
    const std::vector< CWLTestCase::FileData > filesWithContent{
        CWLTestCase::FileData{ "MockFile_1", LintCombine::stringVector{"fileLinter_1"} },
        CWLTestCase::FileData{ "MockFile_2", LintCombine::stringVector{"fileLinter_2"} }
    };
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, /*returnCode*/0 };
}

namespace TestCWL::LintersWorkInParallel {
    const CWLTestCase::Input input{
        LintCombine::stringVector { "--result-yaml=" CURRENT_BINARY_DIR "mockG",
            "--sub-linter=MockLinterWrapper", getRunnerName( "bash" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_1.sh 0 0.5 mes_1 mes_3",
            "--sub-linter=MockLinterWrapper", getRunnerName( "bash" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_2.sh 0 0.25 mes_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const LintCombine::stringVector stdoutData{ "mes_1\nmes_2\nmes_3\n" };
    const LintCombine::stringVector stderrData;
    const std::vector< CWLTestCase::FileData > filesWithContent;
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, /*returnCode*/0 };
}

namespace TestCWL::OneLinterEETC {
    const CWLTestCase::Input input{
        LintCombine::stringVector { "--result-yaml=" CURRENT_BINARY_DIR "mockG",
            "--sub-linter=MockLinterWrapper", getRunnerName( "bash" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
            CURRENT_BINARY_DIR "file_1.txt" },
            LintCombine::stringVector{ CURRENT_BINARY_DIR "file_1.txt" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const LintCombine::stringVector stdoutData;
    const LintCombine::stringVector stderrData;
    const std::vector< CWLTestCase::FileData > filesWithContent;
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, /*returnCode*/0 };
}

namespace TestCWL::TwoLinterEETC {
    const CWLTestCase::Input input{
        LintCombine::stringVector { "--result-yaml=" CURRENT_BINARY_DIR "mockG",
            "--sub-linter=MockLinterWrapper", getRunnerName( "bash" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
            CURRENT_BINARY_DIR "file_1.txt", "--sub-linter=MockLinterWrapper",
            getRunnerName( "bash" ) +
            CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.3 "
            CURRENT_BINARY_DIR "file_2.txt" },
            LintCombine::stringVector{ CURRENT_BINARY_DIR "file_1.txt",
                                       CURRENT_BINARY_DIR "file_2.txt" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const LintCombine::stringVector stdoutData;
    const LintCombine::stringVector stderrData;
    const std::vector< CWLTestCase::FileData > filesWithContent;
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, /*returnCode*/0 };
}

const std::vector< CWLTestCase > CWLTestCaseData = {
    /*0 */    CWLTestCase{TestCWL::L1Terminate::input, TestCWL::L1Terminate::output},
    /*1 */    CWLTestCase{TestCWL::L1Terminate_L2WSFR0::input, TestCWL::L1Terminate_L2WSFR0::output},
    /*2 */    CWLTestCase{TestCWL::L1Terminate_L2Terminate::input, TestCWL::L1Terminate_L2Terminate::output},
    /*3 */    CWLTestCase{TestCWL::L1WSR1::input, TestCWL::L1WSR1::output},
    /*4 */    CWLTestCase{TestCWL::L1WSR1_L2WSFR0::input, TestCWL::L1WSR1_L2WSFR0::output},
    /*5 */    CWLTestCase{TestCWL::L1R1_L2R1::input, TestCWL::L1R1_L2R1::output},
    /*6 */    CWLTestCase{TestCWL::L1WSR0::input, TestCWL::L1WSR0::output},
    /*7 */    CWLTestCase{TestCWL::L1WFR0::input, TestCWL::L1WFR0::output},
    /*8 */    CWLTestCase{TestCWL::L1WSFR0::input, TestCWL::L1WSFR0::output},
    /*9 */    CWLTestCase{TestCWL::L1WSR0_L2WSR0::input, TestCWL::L1WSR0_L2WSR0::output},
    /*10*/    CWLTestCase{TestCWL::L1WFR0_L2WFR0::input, TestCWL::L1WFR0_L2WFR0::output},
    /*11*/    CWLTestCase{TestCWL::L1WSFR0_L2WSFR0::input, TestCWL::L1WSFR0_L2WSFR0::output},
    /*12*/    CWLTestCase{TestCWL::LintersWorkInParallel::input, TestCWL::LintersWorkInParallel::output},
    /*13*/    CWLTestCase{TestCWL::OneLinterEETC::input, TestCWL::OneLinterEETC::output},
    /*14*/    CWLTestCase{TestCWL::TwoLinterEETC::input, TestCWL::TwoLinterEETC::output}
};

BOOST_DATA_TEST_CASE( TestCallAndWaitLinter, CWLTestCaseData, sample ) {
    for( const auto & it : sample.input.fileNamesForLinterEndsEarlyTest ) {
        std::ofstream{ it };
    }
    const auto & correctResult = static_cast< CWLTestCase::Output >( sample.output );
    const StreamCapture stdoutCapture( std::cout );
    const StreamCapture stderrCapture( std::cerr );
    LintCombine::LinterCombine combine(
        sample.input.cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    const auto combineReturnCode = combine.waitLinter();
    const auto stdoutData = stdoutCapture.getBufferData();
    const auto stderrData = stderrCapture.getBufferData();
    BOOST_CHECK( combineReturnCode == correctResult.returnCode );
    for( const auto & it : correctResult.stdoutData ) {
        BOOST_CHECK( stdoutData.find( it ) != std::string::npos );
    }
    for( const auto & it : correctResult.stderrData ) {
        BOOST_CHECK( stderrData.find( it ) != std::string::npos );
    }
    for( const auto & itFile : correctResult.filesWithContent ) {
        BOOST_REQUIRE( std::filesystem::exists( itFile.filename ) );
        std::string fileData;
        getline( std::fstream( itFile.filename ), fileData );
        for( const auto & itFileData : itFile.fileData ) {
            BOOST_CHECK( fileData.find( itFileData ) != std::string::npos );
        }
        std::filesystem::remove( itFile.filename );
    }
    const auto & combineDiagnostics = combine.diagnostics();
    const auto & correctResultDiagnostics = correctResult.diagnostics;
    BOOST_REQUIRE( combineDiagnostics.size() == correctResultDiagnostics.size() );
    for( size_t i = 0; i < correctResultDiagnostics.size(); ++i ) {
        compareDiagnostics( combineDiagnostics[i], correctResultDiagnostics[i] );
    }
    for( const auto & it : sample.input.fileNamesForLinterEndsEarlyTest ) {
        BOOST_CHECK( !std::filesystem::exists( it ) );
        std::filesystem::remove( it );
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestUpdatedYaml )

/*
 * Tests names abbreviations:
 * L<n> means: <n>-th linter
 * DNE means: do/does not exists
 * E means: Exists
 * YP means: YAML-file path
*/

using pairStrStrVec = std::vector< std::pair< std::string, std::string > >;

// UY means UpdatedYaml
struct UYTestCase {
    struct Output {
        Output( const std::vector< LintCombine::Diagnostic > & diagnosticsVal,
                const LintCombine::CallTotals & callTotalsVal,
                const pairStrStrVec & filesForCompareVal )
            : diagnostics( diagnosticsVal ), callTotals( callTotalsVal ),
            filesForCompare( filesForCompareVal ) {}
        std::vector< LintCombine::Diagnostic > diagnostics;
        LintCombine::CallTotals callTotals;
        pairStrStrVec filesForCompare;
    };

    struct Input {
        Input( const LintCombine::stringVector & cmdLineVal,
               LintCombine::LinterFactoryBase & factoryVal =
               LintCombine::MocksLinterFactory::getInstance() )
            : cmdLine( cmdLineVal ), factory( factoryVal ) {}
        LintCombine::stringVector cmdLine;
        LintCombine::LinterFactoryBase & factory;
    };

    UYTestCase( const Input & inputVal,
                const Output & outputVal )
        : input( inputVal ), output( outputVal ) {}

    Input input;
    Output output;
};

std::ostream & operator<<( std::ostream & os, UYTestCase ) {
    return os;
}

namespace TestUY::L1YPDNE {
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "YAML file path \"NotExistentFile\" doesn't exist", "LinterBase", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Updating 1 YAML files failed", "Combine", 1, 0 )
    };
    const LintCombine::CallTotals callTotals{ 0, 1 };
    const pairStrStrVec filesForCompare;
    const UYTestCase::Input input{ LintCombine::stringVector {
        "--result-yaml=" CURRENT_BINARY_DIR "mockG",
        "--sub-linter=MockLinterWrapper", "defaultLinter", "NotExistentFile" } };
    const UYTestCase::Output output{ diagnostics, callTotals, filesForCompare };
}

namespace TestUY::L1YPEmpty {
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "YAML file path \"\" doesn't exist", "LinterBase", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Updating 1 YAML files failed", "Combine", 1, 0 )
    };
    const LintCombine::CallTotals callTotals{ 0, 1 };
    const pairStrStrVec filesForCompare;
    const UYTestCase::Input input{ LintCombine::stringVector {
        "--result-yaml=" CURRENT_BINARY_DIR "mockG",
        "--sub-linter=MockLinterWrapper", "defaultLinter", "" } };
    const UYTestCase::Output output{ diagnostics, callTotals, filesForCompare };
}

namespace TestUY::L1YPEmpty_L2YPE {
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "YAML file path \"\" doesn't exist", "LinterBase", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Updating 1 YAML files failed", "Combine", 1, 0 )
    };
    const LintCombine::CallTotals callTotals{ 1, 1 };
    const pairStrStrVec filesForCompare{
        std::make_pair( CURRENT_SOURCE_DIR "/yamlFiles/linterFile_2.yaml",
                        CURRENT_SOURCE_DIR "/yamlFiles/linterFile_2_save.yaml" ) };
    const UYTestCase::Input input{ LintCombine::stringVector {
        "--result-yaml=" CURRENT_BINARY_DIR "mockG",
        "--sub-linter=MockLinterWrapper", "defaultLinter", "",
        "--sub-linter=MockLinterWrapper", "defaultLinter",
        CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" } };
    const UYTestCase::Output output{ diagnostics, callTotals, filesForCompare };
}

namespace TestUY::TwoLintersYPEmpty {
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "YAML file path \"\" doesn't exist", "LinterBase", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "YAML file path \"\" doesn't exist", "LinterBase", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Updating 2 YAML files failed", "Combine", 1, 0 )
    };
    const LintCombine::CallTotals callTotals{ 0, 2 };
    const pairStrStrVec filesForCompare;
    const UYTestCase::Input input{ LintCombine::stringVector {
        "--result-yaml=" CURRENT_BINARY_DIR "mockG",
        "--sub-linter=MockLinterWrapper", "defaultLinter", "",
        "--sub-linter=MockLinterWrapper", "defaultLinter", "" } };
    const UYTestCase::Output output{ diagnostics, callTotals, filesForCompare };
}

namespace TestUY::L1YPE_L2YPDNE {
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "YAML file path \"NotExistentFile\" doesn't exist", "LinterBase", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Updating 1 YAML files failed", "Combine", 1, 0 )
    };
    const LintCombine::CallTotals callTotals{ 1, 1 };
    const pairStrStrVec filesForCompare{
        std::make_pair( CURRENT_SOURCE_DIR "/yamlFiles/linterFile_2.yaml",
                        CURRENT_SOURCE_DIR "/yamlFiles/linterFile_2_save.yaml" ) };
    const UYTestCase::Input input{ LintCombine::stringVector {
        "--result-yaml=" CURRENT_BINARY_DIR "mockG",
        "--sub-linter=MockLinterWrapper", "defaultLinter", "NotExistentFile",
        "--sub-linter=MockLinterWrapper", "defaultLinter",
        CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" } };
    const UYTestCase::Output output{ diagnostics, callTotals, filesForCompare };
}

namespace TestUY::TwoLintersYPDNE {
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "YAML file path \"NotExistentFile\" doesn't exist", "LinterBase", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "YAML file path \"NotExistentFile\" doesn't exist", "LinterBase", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Updating 2 YAML files failed", "Combine", 1, 0 )
    };
    const LintCombine::CallTotals callTotals{ 0, 2 };
    const pairStrStrVec filesForCompare;
    const UYTestCase::Input input{ LintCombine::stringVector {
        "--result-yaml=" CURRENT_BINARY_DIR "mockG",
        "--sub-linter=MockLinterWrapper", "defaultLinter", "NotExistentFile",
        "--sub-linter=MockLinterWrapper", "defaultLinter", "NotExistentFile" } };
    const UYTestCase::Output output{ diagnostics, callTotals, filesForCompare };
}

namespace TestUY::L1_YPE {
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const LintCombine::CallTotals callTotals{ 1, 0 };
    const pairStrStrVec filesForCompare{
        std::make_pair( CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml",
                        CURRENT_SOURCE_DIR "yamlFiles/linterFile_1_save.yaml" ) };
    const UYTestCase::Input input{ LintCombine::stringVector {
        "--result-yaml=" CURRENT_BINARY_DIR "mockG",
        "--sub-linter=MockLinterWrapper", "defaultLinter",
        CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml" } };
    const UYTestCase::Output output{ diagnostics, callTotals, filesForCompare };
}

namespace TestUY::L1YPE_L2YPE {
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const LintCombine::CallTotals callTotals{ 2, 0 };
    const pairStrStrVec filesForCompare{
        std::make_pair( CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml",
                        CURRENT_SOURCE_DIR "yamlFiles/linterFile_1_save.yaml" ),
        std::make_pair( CURRENT_SOURCE_DIR "yamlFiles/linterFile_2.yaml",
                        CURRENT_SOURCE_DIR "yamlFiles/linterFile_2_save.yaml" ) };
    const UYTestCase::Input input{ LintCombine::stringVector {
        "--result-yaml=" CURRENT_BINARY_DIR "mockG",
        "--sub-linter=MockLinterWrapper", "defaultLinter",
        CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml",
        "--sub-linter=MockLinterWrapper", "defaultLinter",
        CURRENT_SOURCE_DIR "yamlFiles/linterFile_2.yaml" } };
    const UYTestCase::Output output{ diagnostics, callTotals, filesForCompare };
}

namespace TestUY::clangTidyUpdateYaml {
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const LintCombine::CallTotals callTotals{ 1, 0 };
    const pairStrStrVec filesForCompare{
        std::make_pair( CURRENT_SOURCE_DIR "/yamlFiles/linterFile_1.yaml",
                        CURRENT_SOURCE_DIR "/yamlFiles/linterFile_1_result.yaml" ) };
    const UYTestCase::Input input{ LintCombine::stringVector {
        "--result-yaml=" CURRENT_BINARY_DIR "mockG",
        "--sub-linter=clang-tidy", "--export-fixes="
        CURRENT_SOURCE_DIR "/yamlFiles/linterFile_1.yaml" },
        LintCombine::UsualLinterFactory::getInstance() };
    const UYTestCase::Output output{ diagnostics, callTotals, filesForCompare };
}

namespace TestUY::clazyUpdateYaml {
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const LintCombine::CallTotals callTotals{ 1, 0 };
    const pairStrStrVec filesForCompare{
        std::make_pair( CURRENT_SOURCE_DIR "/yamlFiles/linterFile_2.yaml",
                        CURRENT_SOURCE_DIR "/yamlFiles/linterFile_2_result.yaml" ) };
    const UYTestCase::Input input{ LintCombine::stringVector {
        "--result-yaml=" CURRENT_BINARY_DIR "mockG",
        "--sub-linter=clazy", "--export-fixes="
        CURRENT_SOURCE_DIR "/yamlFiles/linterFile_2.yaml" },
        LintCombine::UsualLinterFactory::getInstance() };
    const UYTestCase::Output output{ diagnostics, callTotals, filesForCompare };
}

const std::vector< UYTestCase > UYTestCaseData = {
    /*0 */    UYTestCase{TestUY::L1YPDNE::input, TestUY::L1YPDNE::output},
    /*1 */    UYTestCase{TestUY::L1YPEmpty::input, TestUY::L1YPEmpty::output},
    /*2 */    UYTestCase{TestUY::L1YPEmpty_L2YPE::input, TestUY::L1YPEmpty_L2YPE::output},
    /*3 */    UYTestCase{TestUY::TwoLintersYPEmpty::input, TestUY::TwoLintersYPEmpty::output},
    /*4 */    UYTestCase{TestUY::L1YPE_L2YPDNE::input, TestUY::L1YPE_L2YPDNE::output},
    /*5 */    UYTestCase{TestUY::TwoLintersYPDNE::input, TestUY::TwoLintersYPDNE::output},
    /*6 */    UYTestCase{TestUY::L1_YPE::input, TestUY::L1_YPE::output},
    /*7 */    UYTestCase{TestUY::L1YPE_L2YPE::input, TestUY::L1YPE_L2YPE::output},
    /*8 */    UYTestCase{TestUY::clangTidyUpdateYaml::input, TestUY::clangTidyUpdateYaml::output},
    /*9 */    UYTestCase{TestUY::clazyUpdateYaml::input, TestUY::clazyUpdateYaml::output}
};

BOOST_DATA_TEST_CASE( TestUpdatedYaml, UYTestCaseData, sample ) {
    const auto & correctResult = static_cast< UYTestCase::Output >( sample.output );
    LintCombine::LinterCombine combine(
        sample.input.cmdLine, sample.input.factory );
    recoverYamlFiles();
    const auto callTotals = combine.updateYaml();
    BOOST_CHECK( callTotals.successNum == correctResult.callTotals.successNum );
    BOOST_CHECK( callTotals.failNum == correctResult.callTotals.failNum );
    const auto & combineDiagnostics = combine.diagnostics();
    const auto & correctResultDiagnostics = correctResult.diagnostics;
    BOOST_REQUIRE( combineDiagnostics.size() == correctResultDiagnostics.size() );
    for( size_t i = 0; i < correctResultDiagnostics.size(); ++i ) {
        compareDiagnostics( combineDiagnostics[i], correctResultDiagnostics[i] );
    }
    for( const auto & [lhs, rhs] : correctResult.filesForCompare ) {
        std::ifstream yamlFile( lhs );
        std::ifstream yamlFileSave( rhs );
        std::istream_iterator< char > fileIter( yamlFile ), end;
        std::istream_iterator< char > fileIterSave( yamlFileSave ), endSave;
        BOOST_CHECK_EQUAL_COLLECTIONS( fileIter, end, fileIterSave, endSave );
    }
    recoverYamlFiles();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestMergeYaml )

/*
 * Tests names abbreviations:
 * L<n> means: <n>-th linter
 * DNE means: do/does not exists
 * E means: Exists
 * YP means: YAML-file path
*/

using pairStrStrVec = std::vector< std::pair< std::string, std::string > >;

// MY means MergeYaml
struct MYTestCase {
    struct Output {
        Output( const std::vector< LintCombine::Diagnostic > & diagnosticsVal,
                const pairStrStrVec & filesForCompareVal,
                const std::string & pathToGeneralYAMLVal )
            : diagnostics( diagnosticsVal ), filesForCompare( filesForCompareVal ),
            pathToGeneralYAML( pathToGeneralYAMLVal ) {}
        std::vector< LintCombine::Diagnostic > diagnostics;
        pairStrStrVec filesForCompare;
        std::string pathToGeneralYAML;
    };

    struct Input {
        Input( const LintCombine::stringVector & cmdLineVal )
            : cmdLine( cmdLineVal ) {}
        LintCombine::stringVector cmdLine;
    };

    MYTestCase( const Input & inputVal,
                const Output & outputVal )
        : input( inputVal ), output( outputVal ) {}

    Input input;
    Output output;
};

std::ostream & operator<<( std::ostream & os, MYTestCase ) {
    return os;
}

namespace TestMY::L1YPDNE {
    const MYTestCase::Input input{ LintCombine::stringVector {
        "--result-yaml=" CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
        "--sub-linter=clang-tidy",
        "--export-fixes=" CURRENT_SOURCE_DIR "NotExistentFile" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Warning,
            "Linter's YAML file path \"" CURRENT_SOURCE_DIR "NotExistentFile\" "
            "doesn't exist", "Combine", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "General YAML file isn't created", "Combine", 1, 0 )
    };
    std::string resultPathToGeneralYAML;
    const pairStrStrVec filesForCompare;
    const MYTestCase::Output output{ diagnostics, filesForCompare, resultPathToGeneralYAML };
}

namespace TestMY::L1YPE {
    const MYTestCase::Input input{ LintCombine::stringVector {
        "--result-yaml=" CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
        "--sub-linter=clang-tidy",
        "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    std::string resultPathToGeneralYAML = CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml";
    const pairStrStrVec filesForCompare{
        std::make_pair( CURRENT_SOURCE_DIR "/yamlFiles/combinedResult.yaml",
                        CURRENT_SOURCE_DIR "/yamlFiles/linterFile_1.yaml" ) };
    const MYTestCase::Output output{ diagnostics, filesForCompare, resultPathToGeneralYAML };
}

namespace TestMY::L1YPE_L2YPDNE {
    const MYTestCase::Input input{ LintCombine::stringVector {
        "--result-yaml=" CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
        "--sub-linter=clang-tidy",
        "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml",
        "--sub-linter=clang-tidy",
        "--export-fixes=" CURRENT_SOURCE_DIR "NotExistentFile"} };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Warning,
            "Linter's YAML file path \"" CURRENT_SOURCE_DIR "NotExistentFile\" "
            "doesn't exist", "Combine", 1, 0 )
    };
    std::string resultPathToGeneralYAML = CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml";
    const pairStrStrVec filesForCompare{
        std::make_pair( CURRENT_SOURCE_DIR "/yamlFiles/combinedResult.yaml",
                        CURRENT_SOURCE_DIR "/yamlFiles/linterFile_1.yaml" ) };
    const MYTestCase::Output output{ diagnostics, filesForCompare, resultPathToGeneralYAML };
}

namespace TestMY::TwoLintersYPDNE {
    const MYTestCase::Input input{ LintCombine::stringVector {
        "--result-yaml=" CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
        "--sub-linter=clang-tidy",
        "--export-fixes=" CURRENT_SOURCE_DIR "NotExistentFile",
        "--sub-linter=clang-tidy",
        "--export-fixes=" CURRENT_SOURCE_DIR "NotExistentFile"} };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Warning,
            "Linter's YAML file path \"" CURRENT_SOURCE_DIR "NotExistentFile\" "
            "doesn't exist", "Combine", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Warning,
            "Linter's YAML file path \"" CURRENT_SOURCE_DIR "NotExistentFile\" "
            "doesn't exist", "Combine", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "General YAML file isn't created", "Combine", 1, 0 )
    };
    std::string resultPathToGeneralYAML;
    const pairStrStrVec filesForCompare;
    const MYTestCase::Output output{ diagnostics, filesForCompare, resultPathToGeneralYAML };
}

namespace TestMY::TwoLintersYPE {
    const MYTestCase::Input input{ LintCombine::stringVector {
        "--result-yaml=" CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
        "--sub-linter=clang-tidy",
        "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml",
        "--sub-linter=clang-tidy",
        "--export-fixes=" CURRENT_SOURCE_DIR "yamlFiles/linterFile_2.yaml"} };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    std::string resultPathToGeneralYAML = CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml";
    const pairStrStrVec filesForCompare{
        std::make_pair( CURRENT_SOURCE_DIR "/yamlFiles/combinedResult.yaml",
                        CURRENT_SOURCE_DIR "/yamlFiles/combinedResult_save.yaml" ) };
    const MYTestCase::Output output{ diagnostics, filesForCompare, resultPathToGeneralYAML };
}

const std::vector< MYTestCase > MYTestCaseData = {
    /*0 */    MYTestCase{ TestMY::L1YPDNE::input, TestMY::L1YPDNE::output},
    /*1 */    MYTestCase{ TestMY::L1YPE::input, TestMY::L1YPE::output},
    /*2 */    MYTestCase{ TestMY::L1YPE_L2YPDNE::input, TestMY::L1YPE_L2YPDNE::output},
    /*3 */    MYTestCase{ TestMY::TwoLintersYPDNE::input, TestMY::TwoLintersYPDNE::output},
    /*4 */    MYTestCase{ TestMY::TwoLintersYPE::input, TestMY::TwoLintersYPE::output}
};

BOOST_DATA_TEST_CASE( TestMergeYaml, MYTestCaseData, sample ) {
    const auto & correctResult = static_cast< MYTestCase::Output >( sample.output );
    LintCombine::LinterCombine combine( sample.input.cmdLine );
    recoverYamlFiles();
    BOOST_CHECK( combine.getYamlPath() == correctResult.pathToGeneralYAML );
    if( correctResult.pathToGeneralYAML.empty() ) {
        BOOST_REQUIRE( !std::filesystem::exists( correctResult.pathToGeneralYAML ) );
    }
    else {
        BOOST_REQUIRE( std::filesystem::exists( correctResult.pathToGeneralYAML ) );
    }
    const auto & combineDiagnostics = combine.diagnostics();
    const auto & correctResultDiagnostics = correctResult.diagnostics;
    BOOST_REQUIRE( combineDiagnostics.size() == correctResultDiagnostics.size() );
    for( size_t i = 0; i < correctResultDiagnostics.size(); ++i ) {
        compareDiagnostics( combineDiagnostics[i], correctResultDiagnostics[i] );
    }
    for( const auto & [lhs, rhs] : correctResult.filesForCompare ) {
        std::ifstream yamlFile( lhs );
        std::ifstream yamlFileSave( rhs );
        std::istream_iterator< char > fileIter( yamlFile ), end;
        std::istream_iterator< char > fileIterSave( yamlFileSave ), endSave;
        BOOST_CHECK_EQUAL_COLLECTIONS( fileIter, end, fileIterSave, endSave );
    }
    if( !correctResult.pathToGeneralYAML.empty() ) {
        std::filesystem::remove( correctResult.pathToGeneralYAML );
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestPrepareCommandLine )

/*
 * Tests names abbreviations:
 * L<n> means: <n>-th linter
 * DNE means: do/does not exists
 * E means: Exists
 * YP means: YAML-file path
 * IN means: incorrect name
 * CN means: correct name
 * CT means: clang-tidy
 * CL means: clazy
 * AES means: params value empty after equal sign
 * ASP means: params value empty after space
*/

void compareContainers( const LintCombine::stringVector & lhs,
                        const LintCombine::stringVector & rhs ) {
    BOOST_REQUIRE( lhs.size() == rhs.size() );
    for( size_t i = 0; i < lhs.size(); ++i ) {
        BOOST_CHECK( lhs[i] == rhs[i] );
    }
}

#ifdef _WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

// PCL means PrepareCommandLine
struct PCLTestCase {
    struct Input {
        Input( const LintCombine::stringVector & cmdLineVal )
            : cmdLine( cmdLineVal ) {}
        LintCombine::stringVector cmdLine; // Add IDEName?
    };

    struct Output {
        Output( const std::vector< LintCombine::Diagnostic > & diagnosticsVal,
                const LintCombine::stringVector & resultCmdLineVal,
                const std::optional< bool > & YAMLContainsDocLinkVal )
            : diagnostics( diagnosticsVal ), resultCmdLine( resultCmdLineVal ),
            YAMLContainsDocLink( YAMLContainsDocLinkVal ) {}
        std::vector< LintCombine::Diagnostic > diagnostics;
        LintCombine::stringVector resultCmdLine;
        //bool YAMLContainsDocLink;
        std::optional< bool > YAMLContainsDocLink;
    };

    PCLTestCase( const Input & inputVal,
                 const Output & outputVal )
        : input( inputVal ), output( outputVal ) {}

    Input input;
    Output output;
};

std::ostream & operator<<( std::ostream & os, PCLTestCase ) {
    return os;
}

namespace TestPCL::EmptyCmdLine {
    const PCLTestCase::Input input{ LintCombine::stringVector() };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Command Line is empty", "FactoryPreparer", 1, 0 ) };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::Verbatim_LintersDNE {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--param=value", "-p=val", "--param", "val" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Info,
            "Options were passed verbatim", "VerbatimPreparer", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "No linters specified. Supported linters are: clang-tidy, clazy.",
            "VerbatimPreparer", 1, 0 )
    };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::Verbatim_L1IN {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--sub-linter=Incorrect", "--param=value", "-p=val", "--param", "val" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Info,
            "Options were passed verbatim", "VerbatimPreparer", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Unknown linter name: \"Incorrect\"", "VerbatimPreparer", 1, 0 )
    };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::Verbatim_L1IN_L2IN {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--sub-linter=Incorrect_1", "--sub-linter=Incorrect_2",
        "--param=value", "-p=val", "--param", "val" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Info,
            "Options were passed verbatim", "VerbatimPreparer", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Unknown linter name: \"Incorrect_1\"", "VerbatimPreparer", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Unknown linter name: \"Incorrect_2\"", "VerbatimPreparer", 1, 0 )
    };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::Verbatim_L1CN {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--result-yaml=" CURRENT_BINARY_DIR "file.yaml", "--sub-linter=clazy",
        "--param=value", "-p=val", "--param", "val" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Info,
            "Options were passed verbatim", "VerbatimPreparer", 1, 0 )
    };
    const LintCombine::stringVector resultCmdLine{
        "--result-yaml=" CURRENT_BINARY_DIR "file.yaml", "--sub-linter=clazy",
        "--param=value", "-p=val", "--param", "val" };
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::Verbatim_L1CN_L2CN {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--result-yaml=" CURRENT_BINARY_DIR "file.yaml", "--sub-linter=clazy",
        "--sub-linter=clang-tidy", "--param=value", "-p=val", "--param", "val" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Info,
            "Options were passed verbatim", "VerbatimPreparer", 1, 0 )
    };
    const LintCombine::stringVector resultCmdLine{
        "--result-yaml=" CURRENT_BINARY_DIR "file.yaml", "--sub-linter=clazy",
        "--sub-linter=clang-tidy", "--param=value", "-p=val", "--param", "val" };
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::Verbatim_GeneralYAMLDNE {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--sub-linter=clazy", "--param=value", "-p=val", "--param", "val" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Info,
            "Options were passed verbatim", "VerbatimPreparer", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Path to general YAML-file is not set", "VerbatimPreparer", 1, 0 )
    };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::Verbatim_GeneralYAMLIN {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--result-yaml=\\\\", "--sub-linter=clazy",
        "--param=value", "-p=val", "--param", "val" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Info,
            "Options were passed verbatim", "VerbatimPreparer", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "General YAML-file \"\\\\\" is not creatable", "VerbatimPreparer", 1, 0 )
    };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::UnsupportedIDE {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=shasharper" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "\"shasharper\" is not a supported IDE profile", "FactoryPreparer", 1, 0 )
    };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, std::nullopt };
}

namespace TestPCL::SpecifiedTwice {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase", "-p=pathToCompilationDataBase" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "option '--p' cannot be specified more than once", "BasePreparer", 1, 0 )
    };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::GenYAMLPathEmpty {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Path to yaml-file is empty.", "BasePreparer", 1, 0 )
    };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::CompilationDBEmpty {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "--export-fixes=pathToResultYaml" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Path to compilation database is empty.", "BasePreparer", 1, 0 )
    };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::RequiredOptionsE {
    PCLTestCase::Input input( const std::string & ideName ) {
        return LintCombine::stringVector{ "--ide-profile=" + ideName,
            "-p=pathToCompilationDataBase", "--export-fixes=pathToResultYaml" };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::stringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            LintCombine::Diagnostic( LintCombine::Level::Info,
                "All linters are used", "BasePreparer", 1, 0 ) };
        auto YAMLContainsDocLink = false;
        if( ideName == "CLion" ) { YAMLContainsDocLink = false; }
        if( ideName == "ReSharper" ) { YAMLContainsDocLink = true; }
        return PCLTestCase::Output{ diagnostics, resultCmdLine, YAMLContainsDocLink };
    }
}

namespace TestPCL::CTOptionsPassed {
    PCLTestCase::Input input( const std::string & ideName ) {
        return LintCombine::stringVector{ "--ide-profile=" + ideName,
            "-p=pathToCompilationDataBase", "--export-fixes=pathToResultYaml",
            "--param_1", "@param_2" };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::stringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--param_1", "@param_2",
            "--sub-linter=clazy", "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase" PATH_SEP "diagnosticsClazy.yaml" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            LintCombine::Diagnostic( LintCombine::Level::Info,
                "All linters are used", "BasePreparer", 1, 0 ) };
        auto YAMLContainsDocLink = false;
        if( ideName == "CLion" ) { YAMLContainsDocLink = false; }
        if( ideName == "ReSharper" ) { YAMLContainsDocLink = true; }
        return PCLTestCase::Output{ diagnostics, resultCmdLine, YAMLContainsDocLink };
    }
}

namespace TestPCL::FilesPassed {
    PCLTestCase::Input input( const std::string & ideName ) {
        return LintCombine::stringVector{ "--ide-profile=" + ideName,
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToResultYaml", "file_1.cpp", "file_2.cpp" };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::stringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "file_1.cpp", "file_2.cpp",
            "--sub-linter=clazy", "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase" PATH_SEP "diagnosticsClazy.yaml",
            "file_1.cpp", "file_2.cpp" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            LintCombine::Diagnostic( LintCombine::Level::Info,
                "All linters are used", "BasePreparer", 1, 0 ) };
        auto YAMLContainsDocLink = false;
        if( ideName == "CLion" ) { YAMLContainsDocLink = false; }
        if( ideName == "ReSharper" ) { YAMLContainsDocLink = true; }
        return PCLTestCase::Output{ diagnostics, resultCmdLine, YAMLContainsDocLink };
    }
}

namespace TestPCL::ReSharperHeaderFilterPassed {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--header-filter=file.cpp" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Info,
            "All linters are used", "BasePreparer", 1, 0 )
    };
    const LintCombine::stringVector resultCmdLine{
        "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClangTidy.yaml", "--header-filter=file.cpp",
        "--sub-linter=clazy", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClazy.yaml", "--header-filter=file.cpp" };
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::CLChecksEmptyAES {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--clazy-checks=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the argument for option '--clazy-checks' should follow "
            "immediately after the equal sign", "FactoryPreparer", 1, 0 ) };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::ClangExtraArgsEmptyAES {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--clang-extra-args=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the argument for option '--clang-extra-args' should follow "
            "immediately after the equal sign", "FactoryPreparer", 1, 0 ) };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::ParamsEmptyAES {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--clazy-checks=", "--clang-extra-args=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the argument for option '--clazy-checks' should follow "
            "immediately after the equal sign", "FactoryPreparer", 1, 0 ) };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::CLChecksEmptyASP {
    PCLTestCase::Input input( const std::string & ideName ) {
        return LintCombine::stringVector{ "--ide-profile=" + ideName,
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToResultYaml", "--clazy-checks" };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::stringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            LintCombine::Diagnostic( LintCombine::Level::Info,
                "All linters are used", "BasePreparer", 1, 0 ),
            LintCombine::Diagnostic( LintCombine::Level::Warning,
                "Parameter \"clazy-checks\" was set but "
                 "the parameter's value was not set. "
                 "The parameter will be ignored.", "BasePreparer", 63, 75 ) };
        auto YAMLContainsDocLink = false;
        if( ideName == "CLion" ) { YAMLContainsDocLink = false; }
        if( ideName == "ReSharper" ) { YAMLContainsDocLink = true; }
        return PCLTestCase::Output{ diagnostics, resultCmdLine, YAMLContainsDocLink };
    }
}

namespace TestPCL::ClangXArgsEmptyASP {
    PCLTestCase::Input input( const std::string & ideName ) {
        return LintCombine::stringVector{ "--ide-profile=" + ideName,
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToResultYaml", "--clang-extra-args" };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::stringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            LintCombine::Diagnostic( LintCombine::Level::Info,
                "All linters are used", "BasePreparer", 1, 0 ),
            LintCombine::Diagnostic( LintCombine::Level::Warning,
                "Parameter \"clang-extra-args\" was set but "
                 "the parameter's value was not set. "
                 "The parameter will be ignored.", "BasePreparer", 63, 79 ) };
        auto YAMLContainsDocLink = false;
        if( ideName == "CLion" ) { YAMLContainsDocLink = false; }
        if( ideName == "ReSharper" ) { YAMLContainsDocLink = true; }
        return PCLTestCase::Output{ diagnostics, resultCmdLine, YAMLContainsDocLink };
    }
}

namespace TestPCL::ParamsEmptyASP {
    PCLTestCase::Input input( const std::string & ideName ) {
        return LintCombine::stringVector{ "--ide-profile=" + ideName,
            "-p=pathToCompilationDataBase", "--export-fixes=pathToResultYaml",
            "--clazy-checks", "--clang-extra-args" };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::stringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            LintCombine::Diagnostic( LintCombine::Level::Info,
                "All linters are used", "BasePreparer", 1, 0 ),
            LintCombine::Diagnostic( LintCombine::Level::Warning,
                "Parameter \"clazy-checks\" was set but "
                 "the parameter's value was not set. "
                 "The parameter will be ignored.", "BasePreparer", 63, 75 ),
            LintCombine::Diagnostic( LintCombine::Level::Warning,
                "Parameter \"clang-extra-args\" was set but "
                 "the parameter's value was not set. "
                 "The parameter will be ignored.", "BasePreparer", 78, 94 ) };
        auto YAMLContainsDocLink = false;
        if( ideName == "CLion" ) { YAMLContainsDocLink = false; }
        if( ideName == "ReSharper" ) { YAMLContainsDocLink = true; }
        return PCLTestCase::Output{ diagnostics, resultCmdLine, YAMLContainsDocLink };
    }
}

namespace TestPCL::CLChecksE {
    PCLTestCase::Input input( const std::string & ideName ) {
        return LintCombine::stringVector{ "--ide-profile=" + ideName,
            "-p=pathToCompilationDataBase", "--export-fixes=pathToResultYaml",
            "--clazy-checks", "level0,level1" };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::stringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml", "--checks=level0,level1" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            LintCombine::Diagnostic( LintCombine::Level::Info,
                "All linters are used", "BasePreparer", 1, 0 ) };
        auto YAMLContainsDocLink = false;
        if( ideName == "CLion" ) { YAMLContainsDocLink = false; }
        if( ideName == "ReSharper" ) { YAMLContainsDocLink = true; }
        return PCLTestCase::Output{ diagnostics, resultCmdLine, YAMLContainsDocLink };
    }
}

namespace TestPCL::ClangXArgsE {
    PCLTestCase::Input input( const std::string & ideName ) {
        return LintCombine::stringVector{ "--ide-profile=" + ideName,
            "-p=pathToCompilationDataBase", "--export-fixes=pathToResultYaml",
            "--clang-extra-args=arg_1 arg_2 " };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::stringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml", "--extra-arg=arg_1", "--extra-arg=arg_2" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            LintCombine::Diagnostic( LintCombine::Level::Info,
                "All linters are used", "BasePreparer", 1, 0 ) };
        auto YAMLContainsDocLink = false;
        if( ideName == "CLion" ) { YAMLContainsDocLink = false; }
        if( ideName == "ReSharper" ) { YAMLContainsDocLink = true; }
        return PCLTestCase::Output{ diagnostics, resultCmdLine, YAMLContainsDocLink };
    }
}

namespace TestPCL::ParamsEAES {
    PCLTestCase::Input input( const std::string & ideName ) {
        return LintCombine::stringVector{ "--ide-profile=" + ideName,
            "-p=pathToCompilationDataBase", "--export-fixes=pathToResultYaml",
            "--clazy-checks=level0,level1", "--clang-extra-args=arg_1 arg_2 " };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::stringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml", "--checks=level0,level1",
            "--extra-arg=arg_1", "--extra-arg=arg_2" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            LintCombine::Diagnostic( LintCombine::Level::Info,
                "All linters are used", "BasePreparer", 1, 0 ) };
        auto YAMLContainsDocLink = false;
        if( ideName == "CLion" ) { YAMLContainsDocLink = false; }
        if( ideName == "ReSharper" ) { YAMLContainsDocLink = true; }
        return PCLTestCase::Output{ diagnostics, resultCmdLine, YAMLContainsDocLink };
    }
}

namespace TestPCL::LinterEmptyAES {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the argument for option '--sub-linter' should follow "
            "immediately after the equal sign", "FactoryPreparer", 1, 0 ) };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}


namespace TestPCL::L1FIN_L2EAES {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=IncorrectName_1",
        "--sub-linter=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the argument for option '--sub-linter' should follow "
            "immediately after the equal sign", "FactoryPreparer", 1, 0 ) };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::L1EmptyAES_L2IN {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=",
        "--sub-linter=IncorrectName_1" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the argument for option '--sub-linter' should follow "
            "immediately after the equal sign", "FactoryPreparer", 1, 0 ) };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::L1EmptyAES_L2EmptyAES {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=",
        "--sub-linter=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the argument for option '--sub-linter' should follow "
            "immediately after the equal sign", "FactoryPreparer", 1, 0 ) };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::L1Empty {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the required argument for option '--sub-linter' is missing",
            "BasePreparer", 1, 0 ) };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::L1IN_L2EmptyASP {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter",
        "IncorrectName_1", "--sub-linter" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the required argument for option '--sub-linter' is missing",
            "BasePreparer", 1, 0 ) };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::L1EmptyASP_L2IN {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter",
         "--sub-linter", "IncorrectName_1" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Info,
            "All linters are used",
            "BasePreparer", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Unknown linter name \"--sub-linter\"",
            "BasePreparer", 74, 86 ) };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::L1EmptyASP_L2EmptyASP {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter",
         "--sub-linter" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Info,
            "All linters are used",
            "BasePreparer", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Unknown linter name \"--sub-linter\"",
            "BasePreparer", 74, 86 ) };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::L1IN {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=IncorrectName_1" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Info,
            "All linters are used",
            "BasePreparer", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Unknown linter name \"IncorrectName_1\"",
            "BasePreparer", 74, 89 ) };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::L1IN_L2IN {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml",
        "--sub-linter=IncorrectName_1", "--sub-linter=IncorrectName_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Info,
            "All linters are used",
            "BasePreparer", 1, 0 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Unknown linter name \"IncorrectName_1\"",
            "BasePreparer", 74, 89 ),
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Unknown linter name \"IncorrectName_2\"",
            "BasePreparer", 103, 118 ) };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::LinterIsCT {
    PCLTestCase::Input input( const std::string & ideName ) {
        return LintCombine::stringVector{ "--ide-profile=" + ideName,
            "-p=pathToCompilationDataBase", "--export-fixes=pathToResultYaml",
            "--sub-linter=clang-tidy" };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::stringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml" };

        const std::vector< LintCombine::Diagnostic > diagnostics;
        auto YAMLContainsDocLink = false;
        if( ideName == "CLion" ) { YAMLContainsDocLink = false; }
        if( ideName == "ReSharper" ) { YAMLContainsDocLink = true; }
        return PCLTestCase::Output{ diagnostics, resultCmdLine, YAMLContainsDocLink };
    }
}

namespace TestPCL::LinterIsCL {
    PCLTestCase::Input input( const std::string & ideName ) {
        return LintCombine::stringVector{ "--ide-profile=" + ideName,
            "-p=pathToCompilationDataBase", "--export-fixes=pathToResultYaml",
            "--sub-linter=clazy" };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::stringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml" };

        const std::vector< LintCombine::Diagnostic > diagnostics;
        auto YAMLContainsDocLink = false;
        if( ideName == "CLion" ) { YAMLContainsDocLink = false; }
        if( ideName == "ReSharper" ) { YAMLContainsDocLink = true; }
        return PCLTestCase::Output{ diagnostics, resultCmdLine, YAMLContainsDocLink };
    }
}

namespace TestPCL::AllLintersAES {
    PCLTestCase::Input input( const std::string & ideName ) {
        return LintCombine::stringVector{ "--ide-profile=" + ideName,
            "-p=pathToCompilationDataBase", "--export-fixes=pathToResultYaml",
            "--sub-linter=clang-tidy", "--sub-linter=clazy" };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::stringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml" };

        const std::vector< LintCombine::Diagnostic > diagnostics;
        auto YAMLContainsDocLink = false;
        if( ideName == "CLion" ) { YAMLContainsDocLink = false; }
        if( ideName == "ReSharper" ) { YAMLContainsDocLink = true; }
        return PCLTestCase::Output{ diagnostics, resultCmdLine, YAMLContainsDocLink };
    }
}

namespace TestPCL::AllLintersASP {
    PCLTestCase::Input input( const std::string & ideName ) {
        return LintCombine::stringVector{ "--ide-profile=" + ideName,
            "-p=pathToCompilationDataBase", "--export-fixes=pathToResultYaml",
            "--sub-linter", "clang-tidy", "--sub-linter", "clazy" };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::stringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml" };

        const std::vector< LintCombine::Diagnostic > diagnostics;
        auto YAMLContainsDocLink = false;
        if( ideName == "CLion" ) { YAMLContainsDocLink = false; }
        if( ideName == "ReSharper" ) { YAMLContainsDocLink = true; }
        return PCLTestCase::Output{ diagnostics, resultCmdLine, YAMLContainsDocLink };
    }
}

const std::vector< PCLTestCase > PCLTestCaseData = {
    /*0 */    PCLTestCase{ TestPCL::EmptyCmdLine::input, TestPCL::EmptyCmdLine::output },
    /*1 */    PCLTestCase{ TestPCL::Verbatim_LintersDNE::input, TestPCL::Verbatim_LintersDNE::output },
    /*2 */    PCLTestCase{ TestPCL::Verbatim_L1IN::input, TestPCL::Verbatim_L1IN::output },
    /*3 */    PCLTestCase{ TestPCL::Verbatim_L1IN_L2IN::input, TestPCL::Verbatim_L1IN_L2IN::output },
    /*4 */    PCLTestCase{ TestPCL::Verbatim_L1CN::input, TestPCL::Verbatim_L1CN::output },
    /*5 */    PCLTestCase{ TestPCL::Verbatim_L1CN_L2CN::input, TestPCL::Verbatim_L1CN_L2CN::output },
    /*6 */    PCLTestCase{ TestPCL::Verbatim_GeneralYAMLDNE::input, TestPCL::Verbatim_GeneralYAMLDNE::output },
    /*7 */    PCLTestCase{ TestPCL::Verbatim_GeneralYAMLIN::input, TestPCL::Verbatim_GeneralYAMLIN::output },
    /*8 */    PCLTestCase{ TestPCL::UnsupportedIDE::input, TestPCL::UnsupportedIDE::output },
    /*9 */    PCLTestCase{ TestPCL::SpecifiedTwice::input, TestPCL::SpecifiedTwice::output },
    /*10 */   PCLTestCase{ TestPCL::CompilationDBEmpty::input, TestPCL::CompilationDBEmpty::output },
    /*11*/    PCLTestCase{ TestPCL::GenYAMLPathEmpty::input, TestPCL::GenYAMLPathEmpty::output },
    /*12*/    PCLTestCase{ TestPCL::RequiredOptionsE::input( "ReSharper" ), TestPCL::RequiredOptionsE::output( "ReSharper" ) },
    /*13*/    PCLTestCase{ TestPCL::RequiredOptionsE::input( "CLion" ), TestPCL::RequiredOptionsE::output( "CLion" ) },
    /*14*/    PCLTestCase{ TestPCL::CTOptionsPassed::input( "ReSharper" ), TestPCL::CTOptionsPassed::output( "ReSharper" ) },
    /*15*/    PCLTestCase{ TestPCL::CTOptionsPassed::input( "CLion" ), TestPCL::CTOptionsPassed::output( "CLion" ) },
    /*16*/    PCLTestCase{ TestPCL::FilesPassed::input( "ReSharper" ), TestPCL::FilesPassed::output( "ReSharper" ) },
    /*17*/    PCLTestCase{ TestPCL::FilesPassed::input( "CLion" ), TestPCL::FilesPassed::output( "CLion" ) },
    /*18*/    PCLTestCase{ TestPCL::ReSharperHeaderFilterPassed::input, TestPCL::ReSharperHeaderFilterPassed::output },
    /*19*/    PCLTestCase{ TestPCL::CLChecksEmptyAES::input, TestPCL::CLChecksEmptyAES::output },
    /*20*/    PCLTestCase{ TestPCL::ClangExtraArgsEmptyAES::input, TestPCL::ClangExtraArgsEmptyAES::output },
    /*21*/    PCLTestCase{ TestPCL::ParamsEmptyAES::input, TestPCL::ParamsEmptyAES::output },
    /*22*/    PCLTestCase{ TestPCL::CLChecksEmptyASP::input( "ReSharper" ), TestPCL::CLChecksEmptyASP::output( "ReSharper" ) },
    /*23*/    PCLTestCase{ TestPCL::CLChecksEmptyASP::input( "CLion" ), TestPCL::CLChecksEmptyASP::output( "CLion" ) },
    /*24*/    PCLTestCase{ TestPCL::ClangXArgsEmptyASP::input( "ReSharper" ), TestPCL::ClangXArgsEmptyASP::output( "ReSharper" ) },
    /*25*/    PCLTestCase{ TestPCL::ClangXArgsEmptyASP::input( "CLion" ), TestPCL::ClangXArgsEmptyASP::output( "CLion" ) },
    /*26*/    PCLTestCase{ TestPCL::ParamsEmptyASP::input( "ReSharper" ), TestPCL::ParamsEmptyASP::output( "ReSharper" ) },
    /*27*/    PCLTestCase{ TestPCL::ParamsEmptyASP::input( "CLion" ), TestPCL::ParamsEmptyASP::output( "CLion" ) },
    /*28*/    PCLTestCase{ TestPCL::CLChecksE::input( "ReSharper" ), TestPCL::CLChecksE::output( "ReSharper" ) },
    /*29*/    PCLTestCase{ TestPCL::CLChecksE::input( "CLion" ), TestPCL::CLChecksE::output( "CLion" ) },
    /*30*/    PCLTestCase{ TestPCL::ClangXArgsE::input( "ReSharper" ), TestPCL::ClangXArgsE::output( "ReSharper" ) },
    /*31*/    PCLTestCase{ TestPCL::ClangXArgsE::input( "CLion" ), TestPCL::ClangXArgsE::output( "CLion" ) },
    /*32*/    PCLTestCase{ TestPCL::ParamsEAES::input( "ReSharper" ), TestPCL::ParamsEAES::output( "ReSharper" ) },
    /*33*/    PCLTestCase{ TestPCL::ParamsEAES::input( "CLion" ), TestPCL::ParamsEAES::output( "CLion" ) },
    /*34*/    PCLTestCase{ TestPCL::LinterEmptyAES::input, TestPCL::LinterEmptyAES::output },
    /*35*/    PCLTestCase{ TestPCL::L1FIN_L2EAES::input, TestPCL::L1FIN_L2EAES::output },
    /*36*/    PCLTestCase{ TestPCL::L1EmptyAES_L2IN::input, TestPCL::L1EmptyAES_L2IN::output },
    /*37*/    PCLTestCase{ TestPCL::L1EmptyAES_L2EmptyAES::input, TestPCL::L1EmptyAES_L2EmptyAES::output },
    /*38*/    PCLTestCase{ TestPCL::L1Empty::input, TestPCL::L1Empty::output },
    /*39*/    PCLTestCase{ TestPCL::L1IN_L2EmptyASP::input, TestPCL::L1IN_L2EmptyASP::output },
    /*40*/    PCLTestCase{ TestPCL::L1EmptyASP_L2IN::input, TestPCL::L1EmptyASP_L2IN::output },
    /*41*/    PCLTestCase{ TestPCL::L1EmptyASP_L2EmptyASP::input, TestPCL::L1EmptyASP_L2EmptyASP::output },
    /*42*/    PCLTestCase{ TestPCL::L1IN::input, TestPCL::L1IN::output },
    /*43*/    PCLTestCase{ TestPCL::L1IN_L2IN::input, TestPCL::L1IN_L2IN::output },
    /*44*/    PCLTestCase{ TestPCL::LinterIsCT::input( "ReSharper" ), TestPCL::LinterIsCT::output( "ReSharper" ) },
    /*45*/    PCLTestCase{ TestPCL::LinterIsCT::input( "CLion" ), TestPCL::LinterIsCT::output( "CLion" ) },
    /*46*/    PCLTestCase{ TestPCL::LinterIsCL::input( "ReSharper" ), TestPCL::LinterIsCL::output( "ReSharper" ) },
    /*47*/    PCLTestCase{ TestPCL::LinterIsCL::input( "CLion" ), TestPCL::LinterIsCL::output( "CLion" ) },
    /*48*/    PCLTestCase{ TestPCL::AllLintersAES::input( "ReSharper" ), TestPCL::AllLintersAES::output( "ReSharper" ) },
    /*49*/    PCLTestCase{ TestPCL::AllLintersAES::input( "CLion" ), TestPCL::AllLintersAES::output( "CLion" ) },
    /*50*/    PCLTestCase{ TestPCL::AllLintersASP::input( "ReSharper" ), TestPCL::AllLintersASP::output( "ReSharper" ) },
    /*51*/    PCLTestCase{ TestPCL::AllLintersASP::input( "CLion" ), TestPCL::AllLintersASP::output( "CLion" ) },
};

BOOST_DATA_TEST_CASE( TestMergeYaml, PCLTestCaseData, sample ) {
    const auto & correctResult = static_cast< PCLTestCase::Output >( sample.output );
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto kek = sample.input.cmdLine;
    auto prepareCmdLine = ideTraitsFactory.getPrepareCmdLineInstance( kek );
    if( correctResult.YAMLContainsDocLink.has_value() ) {
        BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->isYamlContainsDocLink() ==
                     correctResult.YAMLContainsDocLink );
    }
    compareContainers( prepareCmdLine->transformCmdLine( kek ),
                       correctResult.resultCmdLine );
    const auto & preparerDiagnostics = prepareCmdLine->diagnostics();
    const auto & correctResultDiagnostics = correctResult.diagnostics;
    BOOST_REQUIRE( preparerDiagnostics.size() == correctResultDiagnostics.size() );
    for( size_t i = 0; i < correctResultDiagnostics.size(); ++i ) {
        compareDiagnostics( preparerDiagnostics[i], correctResultDiagnostics[i] );
    }
}

BOOST_AUTO_TEST_SUITE_END()
