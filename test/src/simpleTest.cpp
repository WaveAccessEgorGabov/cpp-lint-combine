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
    const std::vector < LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::LintersNotSet {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--param_1=value_1", "--param_2=value_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
        "No one linter parsed", "Combine", 1, 0 ) };
    const std::vector < LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::OneLinterNotExists /*L1-NE-*/ {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter=NotExistentLinter" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
        "Unknown linter name: \"NotExistentLinter\"", "Combine", 1, 0 ) };
    const std::vector < LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::FirstLinterNotExistsSecondExists {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter=NotExistentLinter",
                                   "--sub-linter=clazy" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
        "Unknown linter name: \"NotExistentLinter\"", "Combine", 1, 0 ) };
    const std::vector < LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::TwoLintersNotExist {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter=NotExistentLinter_1",
                                   "--sub-linter=NotExistentLinter_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
        "Unknown linter name: \"NotExistentLinter_1\"", "Combine", 1, 0 )
    };
    const std::vector < LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::LintersValueEmptyAfterEqualSign {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the argument for option '--sub-linter' should follow "
            "immediately after the equal sign", "Combine", 1, 0 )
    };
    const std::vector < LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::LintersValueEmptyAfterSpace {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the required argument for option '--sub-linter' "
            "is missing", "Combine", 1, 0 )
    };
    const std::vector < LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::TwoLintersValuesEmptyAfterEqualSign {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter=", "--sub-linter=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the argument for option '--sub-linter' should follow "
            "immediately after the equal sign", "Combine", 1, 0 )
    };
    const std::vector < LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::TwoLintersValuesEmptyAfterSpace {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter", "--sub-linter" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "No one linter parsed", "Combine", 1, 0 )
    };
    const std::vector < LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::GeneralYamlPathValueEmptyAfterEqualSign {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=", "--sub-linter=clazy" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the argument for option '--result-yaml' should follow "
            "immediately after the equal sign", "Combine", 1, 0 )
    };
    const std::vector < LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::GeneralYamlPathValueEmptyAfterSpace {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml", "--sub-linter=clazy",
            "--export-fixes=" CURRENT_BINARY_DIR "mockL" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "General YAML-file \"--sub-linter=clazy\" "
            "is not creatable", "Combine", 1, 0 )
    };
    const std::vector < LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::GeneralYamlPathValueIncorrect {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=\\\\", "--sub-linter=clazy",
            "--export-fixes=" CURRENT_BINARY_DIR "mockL" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "General YAML-file \"\\\\\" is not creatable", "Combine", 1, 0 )
    };
    const std::vector < LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::LinterYamlPathValueEmptyAfterEqualSign {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--sub-linter=clazy", "--export-fixes=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the argument for option '--export-fixes' should "
            "follow immediately after the equal sign", "Combine", 1, 0 )
    };
    const std::vector < LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::LinterYamlPathValueEmptyAfterSpace {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=" CURRENT_BINARY_DIR
            "mockG", "--sub-linter=clazy", "--export-fixes" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "the required argument for option '--export-fixes' "
            "is missing", "clazy-standalone", 1, 0 )
    };
    const std::vector < LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::LinterYamlPathValueIncorrect {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=" CURRENT_BINARY_DIR
            "mockG", "--sub-linter=clazy", "--export-fixes=\\\\" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Linter's YAML-file \"\\\\\" is not creatable", "clazy-standalone", 1, 0 )
    };
    const std::vector < LCCTestCase::LinterData > linterData;
    const LCCTestCase::Output output{ diagnostics, linterData, true };
}

namespace TestLCC::ClazyExists {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=" CURRENT_BINARY_DIR "mockR",
            "--sub-linter=clazy", "--export-fixes=" CURRENT_BINARY_DIR "mockL" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const std::vector < LCCTestCase::LinterData > linterData{
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
    const std::vector < LCCTestCase::LinterData > linterData{
        LCCTestCase::LinterData( "clang-tidy", std::string(),
        CURRENT_BINARY_DIR "mockL" )
    };
    const LCCTestCase::Output output{ diagnostics, linterData, false };
}

namespace TestLCC::ClazyExistsAndHasOptions {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=" CURRENT_BINARY_DIR "mockR",
            "--sub-linter=clazy", "--export-fixes=" CURRENT_BINARY_DIR "mockL",
            "CLParam_1", "CLParam_2"} };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const std::vector < LCCTestCase::LinterData > linterData{
        LCCTestCase::LinterData( "clazy-standalone", "CLParam_1 CLParam_2 ",
        CURRENT_BINARY_DIR "mockL" )
    };
    const LCCTestCase::Output output{ diagnostics, linterData, false };
}

namespace TestLCC::ClangTidyAndClazyExist {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=" CURRENT_BINARY_DIR "mockR",
            "--sub-linter=clang-tidy", "--export-fixes=" CURRENT_BINARY_DIR "mockL",
            "--sub-linter=clazy", "--export-fixes=" CURRENT_BINARY_DIR "mockL" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const std::vector < LCCTestCase::LinterData > linterData{
        LCCTestCase::LinterData( "clang-tidy", std::string(),
        CURRENT_BINARY_DIR "mockL" ),
        LCCTestCase::LinterData( "clazy-standalone", std::string(),
        CURRENT_BINARY_DIR "mockL" )
    };
    const LCCTestCase::Output output{ diagnostics, linterData, false };
}

namespace TestLCC::ClangTidyAndClazyExistAndHaveOptions {
    const LCCTestCase::Input input{
        LintCombine::stringVector{ "--result-yaml=" CURRENT_BINARY_DIR "mockR",
            "--sub-linter=clang-tidy", "--export-fixes=" CURRENT_BINARY_DIR "mockL",
            "CTParam_1", "CTParam_2",
            "--sub-linter=clazy", "--export-fixes=" CURRENT_BINARY_DIR "mockL",
            "CLParam_1", "CLParam_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics;
    const std::vector < LCCTestCase::LinterData > linterData{
        LCCTestCase::LinterData( "clang-tidy", "CTParam_1 CTParam_2 ",
        CURRENT_BINARY_DIR "mockL" ),
        LCCTestCase::LinterData( "clazy-standalone", "CLParam_1 CLParam_2 ",
        CURRENT_BINARY_DIR "mockL" )
    };
    const LCCTestCase::Output output{ diagnostics, linterData, false };
}

const std::vector< LCCTestCase > LCCTestCaseData = {
    /*0*/    { TestLCC::EmptyCmdLine::input, TestLCC::EmptyCmdLine::output },
    /*1 */    { TestLCC::LintersNotSet::input, TestLCC::LintersNotSet::output },
    /*2 */    { TestLCC::OneLinterNotExists::input, TestLCC::OneLinterNotExists::output },
    /*3 */    { TestLCC::FirstLinterNotExistsSecondExists::input, TestLCC::FirstLinterNotExistsSecondExists::output },
    /*4 */    { TestLCC::TwoLintersNotExist::input, TestLCC::TwoLintersNotExist::output },
    /*5 */    { TestLCC::LintersValueEmptyAfterEqualSign::input, TestLCC::LintersValueEmptyAfterEqualSign::output },
    /*6 */    { TestLCC::LintersValueEmptyAfterSpace::input, TestLCC::LintersValueEmptyAfterSpace::output },
    /*7 */    { TestLCC::TwoLintersValuesEmptyAfterEqualSign::input, TestLCC::TwoLintersValuesEmptyAfterEqualSign::output },
    /*8 */    { TestLCC::TwoLintersValuesEmptyAfterSpace::input, TestLCC::TwoLintersValuesEmptyAfterSpace::output },
    /*9 */    { TestLCC::GeneralYamlPathValueEmptyAfterEqualSign::input, TestLCC::GeneralYamlPathValueEmptyAfterEqualSign::output },
    /*10*/    { TestLCC::GeneralYamlPathValueEmptyAfterSpace::input, TestLCC::GeneralYamlPathValueEmptyAfterSpace::output },
    /*11*/    { TestLCC::GeneralYamlPathValueIncorrect::input, TestLCC::GeneralYamlPathValueIncorrect::output },
    /*12*/    { TestLCC::LinterYamlPathValueEmptyAfterEqualSign::input, TestLCC::LinterYamlPathValueEmptyAfterEqualSign::output },
    /*13*/    { TestLCC::LinterYamlPathValueEmptyAfterSpace::input, TestLCC::LinterYamlPathValueEmptyAfterSpace::output },
    /*14*/    { TestLCC::LinterYamlPathValueIncorrect::input, TestLCC::LinterYamlPathValueIncorrect::output },
    /*15*/    { TestLCC::ClazyExists::input, TestLCC::ClazyExists::output },
    /*16*/    { TestLCC::ClangTidyExists::input, TestLCC::ClangTidyExists::output },
    /*17*/    { TestLCC::ClazyExistsAndHasOptions::input, TestLCC::ClazyExistsAndHasOptions::output },
    /*18*/    { TestLCC::ClangTidyAndClazyExist::input, TestLCC::ClangTidyAndClazyExist::output },
    /*19*/    { TestLCC::ClangTidyAndClazyExistAndHaveOptions::input, TestLCC::ClangTidyAndClazyExistAndHaveOptions::output },
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


/*
 * Tests names abbreviations:
 * L<n> means: <n>-th linter
 * WS means: write to streams (stdout, stderr)
 * WF means: write to file
 * WSF means: write to file and to streams (stdout, stderr)
 * R<n> means: return <n>
*/

BOOST_AUTO_TEST_SUITE( TestCallAndWaitLinter )

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

namespace TestCWL::OneLinterEndsEarlierThanCombine {
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

namespace TestCWL::TwoLinterEndEarlierThanCombine {
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
    /*13*/    CWLTestCase{TestCWL::OneLinterEndsEarlierThanCombine::input, TestCWL::OneLinterEndsEarlierThanCombine::output},
    /*14*/    CWLTestCase{TestCWL::TwoLinterEndEarlierThanCombine::input, TestCWL::TwoLinterEndEarlierThanCombine::output}
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

namespace TestUY::LintersYamlPathParamNotExist {
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

namespace TestUY::EmptyLintersYamlPath {
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

namespace TestUY::FirstsYamlPathValueEmptySecondYamlPathExists {
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

namespace TestUY::TwoLintersHaveEmptyYamlPathValue {
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

namespace TestUY::FirstsYamlPathExistsSecondYamlPathExists {
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

namespace TestUY::TwoLintersYamlPathValuesNotExist {
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

namespace TestUY::YamlPathExists {
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

namespace TestUY::TwoLintersHaveExistYamlPath {
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
    /*0 */    UYTestCase{TestUY::LintersYamlPathParamNotExist::input, TestUY::LintersYamlPathParamNotExist::output},
    /*1 */    UYTestCase{TestUY::EmptyLintersYamlPath::input, TestUY::EmptyLintersYamlPath::output},
    /*2 */    UYTestCase{TestUY::FirstsYamlPathValueEmptySecondYamlPathExists::input, TestUY::FirstsYamlPathValueEmptySecondYamlPathExists::output},
    /*3 */    UYTestCase{TestUY::TwoLintersHaveEmptyYamlPathValue::input, TestUY::TwoLintersHaveEmptyYamlPathValue::output},
    /*4 */    UYTestCase{TestUY::FirstsYamlPathExistsSecondYamlPathExists::input, TestUY::FirstsYamlPathExistsSecondYamlPathExists::output},
    /*5 */    UYTestCase{TestUY::TwoLintersYamlPathValuesNotExist::input, TestUY::TwoLintersYamlPathValuesNotExist::output},
    /*6 */    UYTestCase{TestUY::YamlPathExists::input, TestUY::YamlPathExists::output},
    /*7 */    UYTestCase{TestUY::TwoLintersHaveExistYamlPath::input, TestUY::TwoLintersHaveExistYamlPath::output},
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
        std::istream_iterator < char > fileIter( yamlFile ), end;
        std::istream_iterator < char > fileIterSave( yamlFileSave ), endSave;
        BOOST_CHECK_EQUAL_COLLECTIONS( fileIter, end, fileIterSave, endSave );
    }
    recoverYamlFiles();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestMergeYaml )

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

namespace TestMY::OneLintersYamlPathNotExists {
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

namespace TestMY::OneLintersYamlPathExists {
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

namespace TestMY::FirstLintersYamlPathExistSecondLintersYamlPathNotExist {
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

namespace TestMY::TwoLintersYamlPathNotExist {
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

namespace TestMY::TwoLintersYamlPathExist {
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
    /*0 */    MYTestCase{ TestMY::OneLintersYamlPathNotExists::input, TestMY::OneLintersYamlPathNotExists::output},
    /*1 */    MYTestCase{ TestMY::OneLintersYamlPathExists::input, TestMY::OneLintersYamlPathExists::output},
    /*2 */    MYTestCase{ TestMY::FirstLintersYamlPathExistSecondLintersYamlPathNotExist::input, TestMY::FirstLintersYamlPathExistSecondLintersYamlPathNotExist::output},
    /*3 */    MYTestCase{ TestMY::TwoLintersYamlPathNotExist::input, TestMY::TwoLintersYamlPathNotExist::output},
    /*4 */    MYTestCase{ TestMY::TwoLintersYamlPathExist::input, TestMY::TwoLintersYamlPathExist::output},
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
        std::istream_iterator < char > fileIter( yamlFile ), end;
        std::istream_iterator < char > fileIterSave( yamlFileSave ), endSave;
        BOOST_CHECK_EQUAL_COLLECTIONS( fileIter, end, fileIterSave, endSave );
    }
    if( !correctResult.pathToGeneralYAML.empty() ) {
        std::filesystem::remove( correctResult.pathToGeneralYAML );
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestPrepareCommandLine )

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

namespace TestPCL::TwoLintersYamlPathExist {
    const PCLTestCase::Input input{ LintCombine::stringVector() };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Command Line is empty", "FactoryPreparer", 1, 0 ) };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::VerbatimLintersDoNotExist {
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

namespace TestPCL::VerbatimOneLinterWithIncorrectName {
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

namespace TestPCL::VerbatimTwoLintersWithIncorrectNames {
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

namespace TestPCL::VerbatimOneLinterWithCorrectName {
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

namespace TestPCL::VerbatimTwoLintersWithCorrectNames {
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

namespace TestPCL::VerbatimResultYamlPathNotExists {
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

namespace TestPCL::VerbatimInvalidResultYamlPath {
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

namespace TestPCL::PathToCompilationDataBaseEmpty {
    const PCLTestCase::Input input{ LintCombine::stringVector{
        "--ide-profile=ReSharper", "--export-fixes=pathToResultYaml" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        LintCombine::Diagnostic( LintCombine::Level::Error,
            "Path to compilation database is empty.", "BasePreparer", 1, 0 )
    };
    const LintCombine::stringVector resultCmdLine;
    const PCLTestCase::Output output{ diagnostics, resultCmdLine, true };
}

namespace TestPCL::MinimalRequiredOptionsExist {
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

namespace TestPCL::OptionForClangTidyPassed {
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

namespace TestPCL::FilesForAnalyzePassed {
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

namespace TestPCL::ClazyChecksEmptyAfterEqualSign {
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

namespace TestPCL::ClangExtraArgsEmptyAfterEqualSign {
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

namespace TestPCL::AllParamsEmptyAfterEqualSign {
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

namespace TestPCL::ClazyChecksEmptyAfterSpaceHelper {
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

namespace TestPCL::ClangExtraArgsEmptyAfterSpaceHelper {
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

namespace TestPCL::AllParamsEmptyAfterSpaceHelper {
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

namespace TestPCL::ClazyChecksExistHelper {
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

namespace TestPCL::ClangExtraArgsExistHelper {
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

namespace TestPCL::AllParamsExistAfterEqualSignHelper {
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

namespace TestPCL::OneSublinterValueEmptyAfterEqualSign {
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


namespace TestPCL::FirstSubLinterIncorrectSecondValueEmptyAfterEqualSign {
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

namespace TestPCL::FirstSubLinterValueEmptyAfterEqualSignSecondIncorrect {
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

namespace TestPCL::TwoSublinterValuesEmptyAfterEqualSign {
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

namespace TestPCL::OneSublinterEmptyValue {
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

namespace TestPCL::FirstSubLinterIncorrectSecondHasEmptyValueAfterSpace {
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

namespace TestPCL::FirstSubLinterHasEmptyValueAfterSpaceSecondIncorrect {
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

namespace TestPCL::AllSublintersHaveEmptyValueAfterSpace {
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

namespace TestPCL::OneSublinterWithIncorrectName {
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

namespace TestPCL::TwoSublinterWithIncorrectName {
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

namespace TestPCL::SublinterIsClangTidy {
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

namespace TestPCL::SublinterIsClazy {
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

namespace TestPCL::AllLintersAfterEqualSign {
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

namespace TestPCL::AllLintersAfterSpace {
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
    /*0 */    PCLTestCase{ TestPCL::TwoLintersYamlPathExist::input, TestPCL::TwoLintersYamlPathExist::output },
    /*1 */    PCLTestCase{ TestPCL::VerbatimLintersDoNotExist::input, TestPCL::VerbatimLintersDoNotExist::output },
    /*2 */    PCLTestCase{ TestPCL::VerbatimOneLinterWithIncorrectName::input, TestPCL::VerbatimOneLinterWithIncorrectName::output },
    /*3 */    PCLTestCase{ TestPCL::VerbatimTwoLintersWithIncorrectNames::input, TestPCL::VerbatimTwoLintersWithIncorrectNames::output },
    /*4 */    PCLTestCase{ TestPCL::VerbatimOneLinterWithCorrectName::input, TestPCL::VerbatimOneLinterWithCorrectName::output },
    /*5 */    PCLTestCase{ TestPCL::VerbatimTwoLintersWithCorrectNames::input, TestPCL::VerbatimTwoLintersWithCorrectNames::output },
    /*6 */    PCLTestCase{ TestPCL::VerbatimResultYamlPathNotExists::input, TestPCL::VerbatimResultYamlPathNotExists::output },
    /*7 */    PCLTestCase{ TestPCL::VerbatimInvalidResultYamlPath::input, TestPCL::VerbatimInvalidResultYamlPath::output },
    /*8 */    PCLTestCase{ TestPCL::UnsupportedIDE::input, TestPCL::UnsupportedIDE::output },
    /*9 */    PCLTestCase{ TestPCL::SpecifiedTwice::input, TestPCL::SpecifiedTwice::output },
    /*10 */   PCLTestCase{ TestPCL::PathToCompilationDataBaseEmpty::input, TestPCL::PathToCompilationDataBaseEmpty::output },
    /*11*/    PCLTestCase{ TestPCL::GenYAMLPathEmpty::input, TestPCL::GenYAMLPathEmpty::output },
    /*12*/    PCLTestCase{ TestPCL::MinimalRequiredOptionsExist::input( "ReSharper" ), TestPCL::MinimalRequiredOptionsExist::output( "ReSharper" ) },
    /*13*/    PCLTestCase{ TestPCL::MinimalRequiredOptionsExist::input( "CLion" ), TestPCL::MinimalRequiredOptionsExist::output( "CLion" ) },
    /*14*/    PCLTestCase{ TestPCL::OptionForClangTidyPassed::input( "ReSharper" ), TestPCL::OptionForClangTidyPassed::output( "ReSharper" ) },
    /*15*/    PCLTestCase{ TestPCL::OptionForClangTidyPassed::input( "CLion" ), TestPCL::OptionForClangTidyPassed::output( "CLion" ) },
    /*16*/    PCLTestCase{ TestPCL::FilesForAnalyzePassed::input( "ReSharper" ), TestPCL::FilesForAnalyzePassed::output( "ReSharper" ) },
    /*17*/    PCLTestCase{ TestPCL::FilesForAnalyzePassed::input( "CLion" ), TestPCL::FilesForAnalyzePassed::output( "CLion" ) },
    /*18*/    PCLTestCase{ TestPCL::ReSharperHeaderFilterPassed::input, TestPCL::ReSharperHeaderFilterPassed::output },
    /*19*/    PCLTestCase{ TestPCL::ClazyChecksEmptyAfterEqualSign::input, TestPCL::ClazyChecksEmptyAfterEqualSign::output },
    /*20*/    PCLTestCase{ TestPCL::ClangExtraArgsEmptyAfterEqualSign::input, TestPCL::ClangExtraArgsEmptyAfterEqualSign::output },
    /*21*/    PCLTestCase{ TestPCL::AllParamsEmptyAfterEqualSign::input, TestPCL::AllParamsEmptyAfterEqualSign::output },
    /*22*/    PCLTestCase{ TestPCL::ClazyChecksEmptyAfterSpaceHelper::input( "ReSharper" ), TestPCL::ClazyChecksEmptyAfterSpaceHelper::output( "ReSharper" ) },
    /*23*/    PCLTestCase{ TestPCL::ClazyChecksEmptyAfterSpaceHelper::input( "CLion" ), TestPCL::ClazyChecksEmptyAfterSpaceHelper::output( "CLion" ) },
    /*24*/    PCLTestCase{ TestPCL::ClangExtraArgsEmptyAfterSpaceHelper::input( "ReSharper" ), TestPCL::ClangExtraArgsEmptyAfterSpaceHelper::output( "ReSharper" ) },
    /*25*/    PCLTestCase{ TestPCL::ClangExtraArgsEmptyAfterSpaceHelper::input( "CLion" ), TestPCL::ClangExtraArgsEmptyAfterSpaceHelper::output( "CLion" ) },
    /*26*/    PCLTestCase{ TestPCL::AllParamsEmptyAfterSpaceHelper::input( "ReSharper" ), TestPCL::AllParamsEmptyAfterSpaceHelper::output( "ReSharper" ) },
    /*27*/    PCLTestCase{ TestPCL::AllParamsEmptyAfterSpaceHelper::input( "CLion" ), TestPCL::AllParamsEmptyAfterSpaceHelper::output( "CLion" ) },
    /*28*/    PCLTestCase{ TestPCL::ClazyChecksExistHelper::input( "ReSharper" ), TestPCL::ClazyChecksExistHelper::output( "ReSharper" ) },
    /*29*/    PCLTestCase{ TestPCL::ClazyChecksExistHelper::input( "CLion" ), TestPCL::ClazyChecksExistHelper::output( "CLion" ) },
    /*30*/    PCLTestCase{ TestPCL::ClangExtraArgsExistHelper::input( "ReSharper" ), TestPCL::ClangExtraArgsExistHelper::output( "ReSharper" ) },
    /*31*/    PCLTestCase{ TestPCL::ClangExtraArgsExistHelper::input( "CLion" ), TestPCL::ClangExtraArgsExistHelper::output( "CLion" ) },
    /*32*/    PCLTestCase{ TestPCL::AllParamsExistAfterEqualSignHelper::input( "ReSharper" ), TestPCL::AllParamsExistAfterEqualSignHelper::output( "ReSharper" ) },
    /*33*/    PCLTestCase{ TestPCL::AllParamsExistAfterEqualSignHelper::input( "CLion" ), TestPCL::AllParamsExistAfterEqualSignHelper::output( "CLion" ) },
    /*34*/    PCLTestCase{ TestPCL::OneSublinterValueEmptyAfterEqualSign::input, TestPCL::OneSublinterValueEmptyAfterEqualSign::output },
    /*35*/    PCLTestCase{ TestPCL::FirstSubLinterIncorrectSecondValueEmptyAfterEqualSign::input, TestPCL::FirstSubLinterIncorrectSecondValueEmptyAfterEqualSign::output },
    /*36*/    PCLTestCase{ TestPCL::FirstSubLinterValueEmptyAfterEqualSignSecondIncorrect::input, TestPCL::FirstSubLinterValueEmptyAfterEqualSignSecondIncorrect::output },
    /*37*/    PCLTestCase{ TestPCL::TwoSublinterValuesEmptyAfterEqualSign::input, TestPCL::TwoSublinterValuesEmptyAfterEqualSign::output },
    /*38*/    PCLTestCase{ TestPCL::OneSublinterEmptyValue::input, TestPCL::OneSublinterEmptyValue::output },
    /*39*/    PCLTestCase{ TestPCL::FirstSubLinterIncorrectSecondHasEmptyValueAfterSpace::input, TestPCL::FirstSubLinterIncorrectSecondHasEmptyValueAfterSpace::output },
    /*40*/    PCLTestCase{ TestPCL::FirstSubLinterHasEmptyValueAfterSpaceSecondIncorrect::input, TestPCL::FirstSubLinterHasEmptyValueAfterSpaceSecondIncorrect::output },
    /*41*/    PCLTestCase{ TestPCL::AllSublintersHaveEmptyValueAfterSpace::input, TestPCL::AllSublintersHaveEmptyValueAfterSpace::output },
    /*42*/    PCLTestCase{ TestPCL::OneSublinterWithIncorrectName::input, TestPCL::OneSublinterWithIncorrectName::output },
    /*43*/    PCLTestCase{ TestPCL::TwoSublinterWithIncorrectName::input, TestPCL::TwoSublinterWithIncorrectName::output },
    /*44*/    PCLTestCase{ TestPCL::SublinterIsClangTidy::input( "ReSharper" ), TestPCL::SublinterIsClangTidy::output( "ReSharper" ) },
    /*45*/    PCLTestCase{ TestPCL::SublinterIsClangTidy::input( "CLion" ), TestPCL::SublinterIsClangTidy::output( "CLion" ) },
    /*46*/    PCLTestCase{ TestPCL::SublinterIsClazy::input( "ReSharper" ), TestPCL::SublinterIsClazy::output( "ReSharper" ) },
    /*47*/    PCLTestCase{ TestPCL::SublinterIsClazy::input( "CLion" ), TestPCL::SublinterIsClazy::output( "CLion" ) },
    /*48*/    PCLTestCase{ TestPCL::AllLintersAfterEqualSign::input( "ReSharper" ), TestPCL::AllLintersAfterEqualSign::output( "ReSharper" ) },
    /*49*/    PCLTestCase{ TestPCL::AllLintersAfterEqualSign::input( "CLion" ), TestPCL::AllLintersAfterEqualSign::output( "CLion" ) },
    /*50*/    PCLTestCase{ TestPCL::AllLintersAfterSpace::input( "ReSharper" ), TestPCL::AllLintersAfterSpace::output( "ReSharper" ) },
    /*51*/    PCLTestCase{ TestPCL::AllLintersAfterSpace::input( "CLion" ), TestPCL::AllLintersAfterSpace::output( "CLion" ) },
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
