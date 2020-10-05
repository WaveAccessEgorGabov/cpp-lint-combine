#define BOOST_TEST_MODULE CppLintCombineTest

#include "../../src/LinterCombine.h"
#include "../../src/ClazyWrapper.h"
#include "../../src/ClangTidyWrapper.h"
#include "../../src/IdeTraitsFactory.h"
#include "../../src/DiagnosticOutputHelper.h"

#include <boost/test/included/unit_test.hpp>
#include <boost/algorithm/string.hpp>
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
        MockLinterWrapper( const StringVector & cmdLine,
                           LinterFactoryBase::Services & service ) : LinterBase( service ) {
            name = cmdLine[1];
            if( cmdLine.size() >= 3 ) {
                yamlPath = cmdLine[2];
            }
        }

        void updateYamlData( YAML::Node & ) const override {}
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

        std::unique_ptr< LinterItf >
            createLinter( const StringVector & cmdLine ) override {
            if( *cmdLine.begin() == "MockLinterWrapper" ) {
                return std::make_unique< MockLinterWrapper >( cmdLine, getServices() );
            }
            return nullptr;
        }

    private:
        MocksLinterFactory() = default;
    };
}

// Temp directory will contain temporary YAML-files
static std::string generatePathToTempDir() {
    return std::filesystem::temp_directory_path().string() + "/" +
           "cpp-lint-combine_" + std::to_string( std::rand() ) + "/";
}

static void copyRequiredYamlFilesIntoTempDir( const std::string & pathToTempDir ) {
    std::filesystem::create_directory( pathToTempDir );
    const std::string pathToDirWithYamls = CURRENT_SOURCE_DIR "yamlFiles/";
    for( const auto & yamlFileToCopy : { "linterFile_1.yaml", "linterFile_2.yaml" } ) {
        std::filesystem::copy_file(
            pathToDirWithYamls + yamlFileToCopy, pathToTempDir + yamlFileToCopy );
    }
}

static void deleteTempDirWithYamls( const std::string & pathToTempDir ) {
    std::filesystem::remove_all( pathToTempDir );
}

static void checkThatDiagnosticsAreEqual( std::vector< LintCombine::Diagnostic > lhs,
                                          std::vector< LintCombine::Diagnostic > rhs ) {
    std::sort( lhs.begin(), lhs.end() );
    std::sort( rhs.begin(), rhs.end() );
    BOOST_CHECK( lhs == rhs );
}

#ifdef _WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

#define INCORRECT_PATH "<*:?:*>"
const std::string pathToTempDir = generatePathToTempDir() + PATH_SEP;

BOOST_AUTO_TEST_SUITE( TestLinterCombineOutput )

// LCO means LinterCombineOutput
struct LCOTestCase {

    struct Output {
        Output( const std::vector< LintCombine::StringVector > & stdoutDataVal,
                const std::vector< LintCombine::StringVector > & stderrDataVal )
            : stdoutData( stdoutDataVal ), stderrData( stderrDataVal ) {}
        std::vector< LintCombine::StringVector > stdoutData;
        std::vector< LintCombine::StringVector > stderrData;
    };

    struct Input {
        Input( const LintCombine::StringVector & cmdLineVal,
               const std::vector< LintCombine::Diagnostic > & diagnosticsVal )
            : cmdLine( cmdLineVal ), diagnostics( diagnosticsVal ){}
        LintCombine::StringVector cmdLine;
        std::vector< LintCombine::Diagnostic > diagnostics;
    };

    LCOTestCase( const Input & inputVal, const Output & outputVal )
        : input( inputVal ), output( outputVal ) {}

    Input input;
    Output output;
};

std::ostream & operator<<( std::ostream & os, LCOTestCase ) {
    return os;
}

namespace TestLCO::EmptyCmdLine {
    const LCOTestCase::Input input{ /*cmdLine=*/{}, /*diagnostics=*/{} };
    const LCOTestCase::Output output{
        { { "Cpp-lint-combine" }, { "" }, { "--help", "Display all" }, {""} }, /*stderrData*/{} };
}

namespace TestLCO::EmptyCmdLineAndWarningIsPassed {
    const std::vector< LintCombine::Diagnostic > diagnostics = {
        { { LintCombine::Level::Warning, "WarningMessage", "LintCombine", 1, 0 } }
    };
    const LCOTestCase::Input input{ /*cmdLine=*/{}, diagnostics };
    const LCOTestCase::Output output{
        { { "Cpp-lint-combine" }, { "" }, { "--help", "Display all" }, {""} }, /*stderrData*/{} };
}

namespace TestLCO::HelpOptionExists {
    const LCOTestCase::Input input{ /*cmdLine=*/{ "--help" }, /*diagnostics=*/{} };
    const LCOTestCase::Output output{
        { { "Cpp-lint-combine" }, { "" }, { "Usage:" }, { "--help", "Print this message" },
        { "--ide-profile", "ReSharper", "CLion" }, { "--result-yaml", "Path to YAML" },
        { "--sub-linter", "Linter to use" }, { "clang-tidy", "clazy" },
        { "--clazy-checks", "Comma-separated list" },
        { "--clang-extra-args", "Additional argument" }, { "" } },
        /*stderrData*/{} };
}

namespace TestLCO::WarningDiagnosticIsPassed {
    const std::vector< LintCombine::Diagnostic > diagnostics = {
        { { LintCombine::Level::Warning, "WarningMessage", "LintCombine", 1, 0 } } };
    const LCOTestCase::Input input{ /*cmdLine=*/{ "non-empty" }, diagnostics };
    const LCOTestCase::Output output{ /*stdoutData*/{}, { { "Warning: WarningMessage" }, { "" } } };
}

namespace TestLCO::ErrorDiagnosticIsPassed {
    const std::vector< LintCombine::Diagnostic > diagnostics = {
        { { LintCombine::Level::Error, "ErrorMessage", "LintCombine", 1, 0 } }
    };
    const LCOTestCase::Input input{ /*cmdLine=*/{ "non-empty" }, diagnostics };
    const LCOTestCase::Output output{
        /*stdoutData*/{}, { { "Error: ErrorMessage" }, { "" }, { "--help", "Display all" }, {""} } };
}

namespace TestLCO::TwoWarningDiagnosticsArePassed {
    const std::vector< LintCombine::Diagnostic > diagnostics = {
        { LintCombine::Level::Warning, "WarningMessage", "LintCombine", 1, 0 },
        { LintCombine::Level::Warning, "WarningMessage", "LintCombine", 1, 0 }
    };
    const LCOTestCase::Input input{ /*cmdLine=*/{ "non-empty" }, diagnostics };
    const LCOTestCase::Output output{
        /*stdoutData*/{},{ { "Warning: WarningMessage" }, { "Warning: WarningMessage" }, { "" } } };
}

namespace TestLCO::WarningAndErrorDiagnosticsArePassed {
    const std::vector< LintCombine::Diagnostic > diagnostics = {
        { LintCombine::Level::Warning, "WarningMessage", "LintCombine", 1, 0 },
        { LintCombine::Level::Error,   "ErrorMessage", "LintCombine", 1, 0 }
    };
    const LCOTestCase::Input input{ /*cmdLine=*/{ "non-empty" }, diagnostics };
    const LCOTestCase::Output output{
        /*stdoutData*/{},
        { { "Warning: WarningMessage" },
        { "Error: ErrorMessage" }, { "" }, { "--help", "Display all" }, {""} } };
}

namespace TestLCO::TwoErrorDiagnosticsArePassed {
    const std::vector< LintCombine::Diagnostic > diagnostics = {
        { LintCombine::Level::Error, "ErrorMessage", "LintCombine", 1, 0 },
        { LintCombine::Level::Error, "ErrorMessage", "LintCombine", 1, 0 }
    };
    const LCOTestCase::Input input{ /*cmdLine=*/{ "non-empty" }, diagnostics };
    const LCOTestCase::Output output{
        /*stdoutData*/{},
        { { "Error: ErrorMessage" }, { "Error: ErrorMessage" },
        { "" }, { "--help", "Display all" }, { "" } } };
}

const LCOTestCase LCOTestCaseData[] = {
    /*0 */ { TestLCO::EmptyCmdLine::input, TestLCO::EmptyCmdLine::output },
    /*1 */ { TestLCO::EmptyCmdLineAndWarningIsPassed::input, TestLCO::EmptyCmdLineAndWarningIsPassed::output },
    /*2 */ { TestLCO::HelpOptionExists::input, TestLCO::HelpOptionExists::output },
    /*3 */ { TestLCO::WarningDiagnosticIsPassed::input, TestLCO::WarningDiagnosticIsPassed::output },
    /*4 */ { TestLCO::ErrorDiagnosticIsPassed::input, TestLCO::ErrorDiagnosticIsPassed::output },
    /*5 */ { TestLCO::TwoWarningDiagnosticsArePassed::input, TestLCO::TwoWarningDiagnosticsArePassed::output },
    /*6 */ { TestLCO::WarningAndErrorDiagnosticsArePassed::input, TestLCO::WarningAndErrorDiagnosticsArePassed::output },
    /*7 */ { TestLCO::TwoErrorDiagnosticsArePassed::input, TestLCO::TwoErrorDiagnosticsArePassed::output },
};

void testLinterCombineOutputHelper( const bool isStdout, const LCOTestCase::Input & input,
                                    const std::vector< LintCombine::StringVector > & output ) {
    const LintCombine::DiagnosticOutputHelper diagnosticWorker( input.cmdLine );
    std::vector< std::string > outputSplitByLine;
    {
        const StreamCapture stdoutCapture( std::cout );
        const StreamCapture stderrCapture( std::cerr );
        diagnosticWorker.printDiagnostics( // TODO: why const_cast doesn't work here, but static_cast does ???
            static_cast< std::vector< LintCombine::Diagnostic > >( input.diagnostics ) );
        const auto streamOutput = isStdout ? stdoutCapture.getBufferData() : stderrCapture.getBufferData();
        split( outputSplitByLine, streamOutput, boost::is_any_of( "\n" ) );
        if( outputSplitByLine.size() == 1 )
            outputSplitByLine.clear();
    }
    BOOST_REQUIRE( outputSplitByLine.size() == output.size() );
    for( size_t i = 0; i < outputSplitByLine.size(); ++i ) {
        for( size_t j = 0; j < output[i].size(); ++j ) {
            BOOST_CHECK( outputSplitByLine[i].find( output[i][j] ) != std::string::npos );
        }
    }
}

// TODO: Reduce code duplication
BOOST_DATA_TEST_CASE( TestLinterCombineOutput, LCOTestCaseData, sample ) {
    testLinterCombineOutputHelper( /*stdout=*/ true, sample.input, sample.output.stdoutData );
    testLinterCombineOutputHelper( /*stderr=*/false, sample.input, sample.output.stderrData );
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestUsualLinterFactory )

// ULF means UsualLinterFactory
struct ULFTestCase {

    struct Output {
        Output( const std::string & returnedObjectTypeNameVal )
            : returnedObjectTypeName( returnedObjectTypeNameVal ) {}
        std::string returnedObjectTypeName;
    };

    struct Input {
        Input( const LintCombine::StringVector & cmdLineVal )
            : cmdLine( cmdLineVal ) {}
        LintCombine::StringVector cmdLine;
    };

    ULFTestCase( const Input & inputVal, const Output & outputVal )
        : input( inputVal ), output( outputVal ) {}

    Input input;
    Output output;
};

std::ostream & operator<<( std::ostream & os, ULFTestCase ) {
    return os;
}

namespace TestULF::EmptyCmdLine {
    const ULFTestCase::Input input{ {} };
    const ULFTestCase::Output output{ {} };
}

namespace TestULF::UnknownLinter {
    const ULFTestCase::Input input{ { "--sub-linter=Unknown" } };
    const ULFTestCase::Output output{ {} };
}

namespace TestULF::LinterIsClazy {
    const ULFTestCase::Input input{ { "clazy", "--export-fixes=" + pathToTempDir + "mock" } };
    const ULFTestCase::Output output{ "ClazyWrapper" };
}

namespace TestULF::LinterIsClangTidy {
    const ULFTestCase::Input input{ { "clang-tidy", "--export-fixes=" + pathToTempDir + "mock" } };
    const ULFTestCase::Output output{ "ClangTidyWrapper" };
}

namespace TestULF::LinterIsCombine {
    const ULFTestCase::Input input{
        { "LinterCombine", "--result-yaml=" + pathToTempDir + "mock",
          "--sub-linter=clazy", "--export-fixes=" + pathToTempDir + "mock" } };
    const ULFTestCase::Output output{ "LinterCombine" };
}

const ULFTestCase ULFTestCaseData[] = {
    /*0 */ { TestULF::EmptyCmdLine::input, TestULF::EmptyCmdLine::output },
    /*1 */ { TestULF::UnknownLinter::input, TestULF::UnknownLinter::output },
    /*2 */ { TestULF::LinterIsClazy::input, TestULF::LinterIsClazy::output },
    /*3 */ { TestULF::LinterIsClangTidy::input, TestULF::LinterIsClangTidy::output },
    /*4 */ { TestULF::LinterIsCombine::input, TestULF::LinterIsCombine::output },
};

BOOST_DATA_TEST_CASE( TestUsualLinterFactory, ULFTestCaseData, sample ) {
    const auto & correctResult = static_cast< ULFTestCase::Output >( sample.output );
    auto linter = LintCombine::UsualLinterFactory::getInstance().createLinter( sample.input.cmdLine );
    if( !correctResult.returnedObjectTypeName.empty() ) {
        BOOST_CHECK( linter );
        if( correctResult.returnedObjectTypeName == "LinterCombine" ) {
            BOOST_CHECK( dynamic_cast< LintCombine::LinterCombine * >( linter.get() ) );
        }
        else if( correctResult.returnedObjectTypeName == "ClazyWrapper" ) {
            BOOST_CHECK( dynamic_cast< LintCombine::ClazyWrapper * >( linter.get() ) );
        }
        else if( correctResult.returnedObjectTypeName == "ClangTidyWrapper" ) {
            BOOST_CHECK( dynamic_cast< LintCombine::ClangTidyWrapper * >( linter.get() ) );
        }
        else { BOOST_CHECK( !"returnedObjectTypeName contains unknown class name" ); }
    }
    else { BOOST_CHECK( !linter ); }
}

BOOST_AUTO_TEST_SUITE_END()

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
        LinterData( const std::string & nameVal, const std::string & optionsVal,
                    const std::string & yamlPathVal )
            : name( nameVal ), options( optionsVal ), yamlPath( yamlPathVal ) {}
        std::string name;
        std::string options;
        std::string yamlPath;
    };

    struct Output {
        Output( const std::vector< LintCombine::Diagnostic > & diagnosticsVal,
                const std::vector< LinterData > & linterDataVal, const bool exceptionOccuredVal )
            : diagnostics( diagnosticsVal ), linterData( linterDataVal ),
              exceptionOccurred( exceptionOccuredVal ) {}
        std::vector< LintCombine::Diagnostic > diagnostics;
        std::vector< LinterData > linterData;
        bool exceptionOccurred = false;
    };

    struct Input {
        Input( const LintCombine::StringVector & cmdLineVal ) : cmdLine( cmdLineVal ) {}
        LintCombine::StringVector cmdLine;
    };

    LCCTestCase( const Input & inputVal, const Output & outputVal )
        : input( inputVal ), output( outputVal ) {}

    Input input;
    Output output;
};

std::ostream & operator<<( std::ostream & os, LCCTestCase ) {
    return os;
}

namespace TestLCC::EmptyCmdLine {
    const LCCTestCase::Input input{ {} };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error, "Command Line is empty", "LintCombine", 1, 0 } };
    const LCCTestCase::Output output{ diagnostics, /*linterData=*/{}, /*exceptionOccurred=*/true };
}

namespace TestLCC::NoLintersSet {
    const LCCTestCase::Input input{ { "--param_1=value_1", "--param_2=value_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "No linters specified. Use --sub-linter, see --help.", "LintCombine", 1, 0 } };
    const LCCTestCase::Output output{ diagnostics, /*linterData=*/{}, /*exceptionOccurred=*/true };
}

namespace TestLCC::L1DNE {
    const LCCTestCase::Input input{ { "--sub-linter=NotExistentLinter" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "Unknown linter name: \"NotExistentLinter\"", "LintCombine", 1, 0 } };
    const LCCTestCase::Output output{ diagnostics, /*linterData=*/{}, /*exceptionOccurred=*/true };
}

namespace TestLCC::L1DNE_L2E {
    const LCCTestCase::Input input{ { "--sub-linter=NotExistentLinter", "--sub-linter=clazy" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "Unknown linter name: \"NotExistentLinter\"", "LintCombine", 1, 0 } };
    const LCCTestCase::Output output{ diagnostics, /*linterData=*/{}, /*exceptionOccurred=*/true };
}

namespace TestLCC::L1DNE_L2DNE {
    const LCCTestCase::Input input{
        { "--sub-linter=NotExistentLinter_1", "--sub-linter=NotExistentLinter_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "Unknown linter name: \"NotExistentLinter_1\"", "LintCombine", 1, 0 } };
    const LCCTestCase::Output output{ diagnostics, /*linterData=*/{}, /*exceptionOccurred=*/true };
}

namespace TestLCC::OneLintersVEAES {
    const LCCTestCase::Input input{ { "--sub-linter=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "the argument for option '--sub-linter' should follow immediately after the equal sign",
          "LintCombine", 1, 0 } };
    const LCCTestCase::Output output{ diagnostics, /*linterData=*/{}, /*exceptionOccurred=*/true };
}

namespace TestLCC::OneLintersVEASP {
    const LCCTestCase::Input input{ { "--sub-linter" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "the required argument for option '--sub-linter' is missing", "LintCombine", 1, 0 } };
    const LCCTestCase::Output output{ diagnostics, /*linterData=*/{}, /*exceptionOccurred=*/true };
}

namespace TestLCC::TwoLintersVEAES {
    const LCCTestCase::Input input{ { "--sub-linter=", "--sub-linter=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "the argument for option '--sub-linter' should follow immediately after the equal sign",
          "LintCombine", 1, 0 } };
    const LCCTestCase::Output output{ diagnostics, /*linterData=*/{}, /*exceptionOccurred=*/true };
}

namespace TestLCC::TwoLintersVEASP {
    const LCCTestCase::Input input{ { "--sub-linter", "--sub-linter" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "No linters specified. Use --sub-linter, see --help.", "LintCombine", 1, 0 } };
    const LCCTestCase::Output output{ diagnostics, /*linterData=*/{}, /*exceptionOccurred=*/true };
}

namespace TestLCC::CombinedYamlPathVEAES {
    const LCCTestCase::Input input{ { "--result-yaml=", "--sub-linter=clazy" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "the argument for option '--result-yaml' should follow immediately after the equal sign",
          "LintCombine", 1, 0 } };
    const LCCTestCase::Output output{ diagnostics, /*linterData=*/{}, /*exceptionOccurred=*/true };
}

namespace TestLCC::CombinedYamlPathVEASP {
    const LCCTestCase::Input input{
        { "--result-yaml", "--sub-linter=clazy", "--export-fixes=" + pathToTempDir + "mockL" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "Combined YAML-file \"--sub-linter=clazy\" is not creatable", "LintCombine", 1, 0 } };
    const LCCTestCase::Output output{ diagnostics, /*linterData=*/{}, /*exceptionOccurred=*/true };
}

namespace TestLCC::CombinedYamlPathVI {
    const LCCTestCase::Input input{
        { "--result-yaml=" INCORRECT_PATH, "--sub-linter=clazy",
          "--export-fixes=" + pathToTempDir + "mockL" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "Combined YAML-file \"" INCORRECT_PATH "\" is not creatable", "LintCombine", 1, 0 } };
    const LCCTestCase::Output output{ diagnostics, /*linterData=*/{}, /*exceptionOccurred=*/true };
}

namespace TestLCC::LinterYamlPathVEAES {
    const LCCTestCase::Input input{ { "--sub-linter=clazy", "--export-fixes=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "the argument for option '--export-fixes' should follow immediately after the equal sign",
          "LintCombine", 1, 0 } };
    const LCCTestCase::Output output{ diagnostics, /*linterData=*/{}, /*exceptionOccurred=*/true };
}

namespace TestLCC::LinterYamlPathVEASP {
    const LCCTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG", "--sub-linter=clazy", "--export-fixes" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "the required argument for option '--export-fixes' is missing", "clazy-standalone", 1, 0 } };
    const LCCTestCase::Output output{ diagnostics, /*linterData=*/{}, /*exceptionOccurred=*/true };
}

namespace TestLCC::LinterYamlPathVI {
    const LCCTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=clazy", "--export-fixes=" INCORRECT_PATH } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "Linter's YAML-file \"" INCORRECT_PATH "\" is not creatable", "clazy-standalone", 1, 0 } };
    const LCCTestCase::Output output{ diagnostics, /*linterData=*/{}, /*exceptionOccurred=*/true };
}

namespace TestLCC::ClazyExists {
    const LCCTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockR",
          "--sub-linter=clazy", "--export-fixes=" + pathToTempDir + "mockL" } };
    const std::vector< LCCTestCase::LinterData > linterData{
        { "clazy-standalone", /*options=*/{}, pathToTempDir + "mockL" } };
    const LCCTestCase::Output output{ /*diagnostics=*/{}, linterData, /*exceptionOccurred=*/false };
}

namespace TestLCC::ClangTidyExists {
    const LCCTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockR",
          "--sub-linter=clang-tidy", "--export-fixes=" + pathToTempDir + "mockL" } };
    const std::vector< LCCTestCase::LinterData > linterData{
        { "clang-tidy", /*options=*/{}, pathToTempDir + "mockL" } };
    const LCCTestCase::Output output{ /*diagnostics=*/{}, linterData, /*exceptionOccurred=*/false };
}

namespace TestLCC::ClazyEWithOptions {
    const LCCTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockR",
          "--sub-linter=clazy", "--export-fixes=" + pathToTempDir + "mockL", "CLParam_1", "CLParam_2"} };
    const std::vector< LCCTestCase::LinterData > linterData{
        { "clazy-standalone", "CLParam_1 CLParam_2 ", pathToTempDir + "mockL" } };
    const LCCTestCase::Output output{ /*diagnostics=*/{}, linterData, /*exceptionOccurred=*/false };
}

namespace TestLCC::ClangTidyAndClazyE {
    const LCCTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockR",
          "--sub-linter=clang-tidy", "--export-fixes=" + pathToTempDir + "mockL",
          "--sub-linter=clazy", "--export-fixes=" + pathToTempDir + "mockL" } };
    const std::vector< LCCTestCase::LinterData > linterData{
        { "clang-tidy",/*options=*/{}, pathToTempDir + "mockL" },
        { "clazy-standalone", /*options=*/{}, pathToTempDir + "mockL" } };
    const LCCTestCase::Output output{ /*diagnostics=*/{}, linterData, /*exceptionOccurred=*/false };
}

namespace TestLCC::ClangTidyAndClazyEWithOptions {
    const LCCTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockR",
          "--sub-linter=clang-tidy", "--export-fixes=" + pathToTempDir + "mockL", "CTParam_1", "CTParam_2",
          "--sub-linter=clazy", "--export-fixes=" + pathToTempDir + "mockL", "CLParam_1", "CLParam_2" } };
    const std::vector< LCCTestCase::LinterData > linterData{
        { "clang-tidy", "CTParam_1 CTParam_2 ", pathToTempDir + "mockL" },
        { "clazy-standalone", "CLParam_1 CLParam_2 ", pathToTempDir + "mockL" } };
    const LCCTestCase::Output output{ /*diagnostics=*/{}, linterData, /*exceptionOccurred=*/false };
}

const LCCTestCase LCCTestCaseData[] = {
    /*0 */ { TestLCC::EmptyCmdLine::input, TestLCC::EmptyCmdLine::output },
    /*1 */ { TestLCC::NoLintersSet::input, TestLCC::NoLintersSet::output },
    /*2 */ { TestLCC::L1DNE::input, TestLCC::L1DNE::output },
    /*3 */ { TestLCC::L1DNE_L2E::input, TestLCC::L1DNE_L2E::output },
    /*4 */ { TestLCC::L1DNE_L2DNE::input, TestLCC::L1DNE_L2DNE::output },
    /*5 */ { TestLCC::OneLintersVEAES::input, TestLCC::OneLintersVEAES::output },
    /*6 */ { TestLCC::OneLintersVEASP::input, TestLCC::OneLintersVEASP::output },
    /*7 */ { TestLCC::TwoLintersVEAES::input, TestLCC::TwoLintersVEAES::output },
    /*8 */ { TestLCC::TwoLintersVEASP::input, TestLCC::TwoLintersVEASP::output },
    /*9 */ { TestLCC::CombinedYamlPathVEAES::input, TestLCC::CombinedYamlPathVEAES::output },
    /*10*/ { TestLCC::CombinedYamlPathVEASP::input, TestLCC::CombinedYamlPathVEASP::output },
    /*11*/ { TestLCC::CombinedYamlPathVI::input, TestLCC::CombinedYamlPathVI::output },
    /*12*/ { TestLCC::LinterYamlPathVEAES::input, TestLCC::LinterYamlPathVEAES::output },
    /*13*/ { TestLCC::LinterYamlPathVEASP::input, TestLCC::LinterYamlPathVEASP::output },
    /*14*/ { TestLCC::LinterYamlPathVI::input, TestLCC::LinterYamlPathVI::output },
    /*15*/ { TestLCC::ClazyExists::input, TestLCC::ClazyExists::output },
    /*16*/ { TestLCC::ClangTidyExists::input, TestLCC::ClangTidyExists::output },
    /*17*/ { TestLCC::ClazyEWithOptions::input, TestLCC::ClazyEWithOptions::output },
    /*18*/ { TestLCC::ClangTidyAndClazyE::input, TestLCC::ClangTidyAndClazyE::output },
    /*19*/ { TestLCC::ClangTidyAndClazyEWithOptions::input, TestLCC::ClangTidyAndClazyEWithOptions::output },
};

BOOST_DATA_TEST_CASE( TestLinterCombineConstructor, LCCTestCaseData, sample ) {
    const auto & correctResult = static_cast< LCCTestCase::Output >( sample.output );
    if( correctResult.exceptionOccurred ) {
        try {
            LintCombine::LinterCombine combine{ sample.input.cmdLine };
        }
        catch( const LintCombine::Exception & ex ) {
            checkThatDiagnosticsAreEqual( ex.diagnostics(), correctResult.diagnostics );
        }
    }
    else {
        const LintCombine::LinterCombine combine( sample.input.cmdLine );
        BOOST_CHECK( combine.numLinters() == correctResult.linterData.size() );
        for( size_t i = 0; i < combine.numLinters(); ++i ) {
            const auto & linter = dynamic_cast< LintCombine::LinterBase * >( combine.linterAt( i ).get() );
            BOOST_CHECK( correctResult.linterData[i].name == linter->getName() );
            BOOST_CHECK( correctResult.linterData[i].options == linter->getOptions() );
            BOOST_CHECK( correctResult.linterData[i].yamlPath == linter->getYamlPath() );
        }
        checkThatDiagnosticsAreEqual( combine.diagnostics(), correctResult.diagnostics );
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
                  const LintCombine::StringVector & fileDataVal )
            : filename( filenameVal ), fileData( fileDataVal ) {}
        std::string filename;
        LintCombine::StringVector fileData;
    };

    struct Output {
        Output( const std::vector< LintCombine::Diagnostic > & diagnosticsVal,
                const LintCombine::StringVector & stdoutDataVal,
                const LintCombine::StringVector & stderrDataVal,
                const std::vector< FileData > & filesWithContentVal, const int returnCodeVal )
            : diagnostics( diagnosticsVal ), stdoutData( stdoutDataVal ),
              stderrData( stderrDataVal ), filesWithContent( filesWithContentVal ),
              returnCode( returnCodeVal ) {}
        std::vector< LintCombine::Diagnostic > diagnostics;
        LintCombine::StringVector stdoutData;
        LintCombine::StringVector stderrData;
        std::vector< FileData > filesWithContent;
        int returnCode;
    };

    struct Input {
        Input( const LintCombine::StringVector & cmdLineVal,
               const LintCombine::StringVector & fileNamesForLinterEndsEarlyTestVal = {} )
            : cmdLine( cmdLineVal ),
            fileNamesForLinterEndsEarlyTest( fileNamesForLinterEndsEarlyTestVal ) {}
        LintCombine::StringVector cmdLine;
        // files to check that linter ends early than combine
        LintCombine::StringVector fileNamesForLinterEndsEarlyTest;
    };

    CWLTestCase( const Input & inputVal, const Output & outputVal )
        : input( inputVal ), output( outputVal ) {}

    Input input;
    Output output;
};

std::ostream & operator<<( std::ostream & os, CWLTestCase ) {
    return os;
}

std::string getRunnerName( const std::string & shellName ) {
    if constexpr( BOOST_OS_WINDOWS ) {
        if( shellName == "cmd" ) { return {}; }
        if( shellName == "bash" ) { return "sh.exe "; }
    }
    if constexpr( BOOST_OS_LINUX ) { return "sh "; }
    return {};
}

std::string getScriptExtension() {
    if constexpr( BOOST_OS_WINDOWS ) { return ".cmd"; }
    else { return ".sh"; }
}

namespace TestCWL::L1Terminate {
    const CWLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", getRunnerName( "bash" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStdoutAndTerminate.sh stdoutLinter_1" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error, "All linters failed while running", "LintCombine", 1, 0 } };
    const LintCombine::StringVector stdoutData{ "stdoutLinter_1" };
    const CWLTestCase::Output output{
        diagnostics, stdoutData, /*stderrData=*/{}, /*filesWithContent=*/{}, LintCombine::AllLintersFailed };
}

namespace TestCWL::L1Terminate_L2WSFR0 {
    const CWLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", getRunnerName( "bash" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStdoutAndTerminate.sh "
          "stdoutLinter_1", "--sub-linter=MockLinterWrapper",
          getRunnerName( "cmd" ) + CURRENT_SOURCE_DIR
          "mockPrograms/mockWriteToStreamsAndToFile" +
          getScriptExtension() + " 0 " + pathToTempDir +
          "MockFile_2 fileLinter_2 stdoutLinter_2 stderrLinter_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Warning, "Some linters failed while running", "LintCombine", 1, 0 } };
    const LintCombine::StringVector stdoutData{ "stdoutLinter_1", "stdoutLinter_2" };
    const LintCombine::StringVector stderrData{ "stderrLinter_2" };
    const std::vector< CWLTestCase::FileData > filesWithContent{
        { pathToTempDir + "MockFile_2", { "fileLinter_2" } } };
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, LintCombine::SomeLintersFailed };
}

namespace TestCWL::L1Terminate_L2Terminate {
    const CWLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", getRunnerName( "bash" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStdoutAndTerminate.sh "
          "stdoutLinter_1", "--sub-linter=MockLinterWrapper",
          getRunnerName( "bash" ) + CURRENT_SOURCE_DIR
          "mockPrograms/mockWriteToStdoutAndTerminate.sh stdoutLinter_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error, "All linters failed while running", "LintCombine", 1, 0 } };
    const LintCombine::StringVector stdoutData{ "stdoutLinter_1", "stdoutLinter_2" };
    const CWLTestCase::Output output{
        diagnostics, stdoutData, /*stderrData*/{}, /*filesWithContent*/{}, LintCombine::AllLintersFailed };
}

namespace TestCWL::L1WSR1 {
    const CWLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreams" +
          getScriptExtension() + " 1 stdoutLinter_1 stderrLinter_1" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error, "All linters failed while running", "LintCombine", 1, 0 } };
    const LintCombine::StringVector stdoutData{ "stdoutLinter_1" };
    const LintCombine::StringVector stderrData{ "stderrLinter_1" };
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, /*filesWithContent*/{}, LintCombine::AllLintersFailed };
}

namespace TestCWL::L1WSR1_L2WSFR0 {
    const CWLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreams" +
          getScriptExtension() + " 1 stdoutLinter_1 stderrLinter_1",
          "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreamsAndToFile" +
          getScriptExtension() + " 0 " + pathToTempDir + "MockFile_2 "
          "fileLinter_2 stdoutLinter_2 stderrLinter_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Warning, "Some linters failed while running", "LintCombine", 1, 0 } };
    const LintCombine::StringVector stdoutData{ "stdoutLinter_1", "stdoutLinter_2" };
    const LintCombine::StringVector stderrData{ "stderrLinter_1", "stderrLinter_2" };
    const std::vector< CWLTestCase::FileData > filesWithContent{
        { pathToTempDir + "MockFile_2", { "fileLinter_2" } } };
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, filesWithContent, LintCombine::SomeLintersFailed };
}

namespace TestCWL::L1R1_L2R1 {
    const CWLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreams" +
          getScriptExtension() + " 1 stdoutLinter_1 stderrLinter_1",
          "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreams" +
          getScriptExtension() + " 2 stdoutLinter_2 stderrLinter_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error, "All linters failed while running", "LintCombine", 1, 0 } };
    const LintCombine::StringVector stdoutData{ "stdoutLinter_1", "stdoutLinter_2" };
    const LintCombine::StringVector stderrData{ "stderrLinter_1", "stderrLinter_2" };
    const CWLTestCase::Output output{
        diagnostics, stdoutData, stderrData, /*filesWithContent*/{}, LintCombine::AllLintersFailed };
}

namespace TestCWL::L1WSR0 {
    const CWLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreams" +
          getScriptExtension() + " 0 stdoutLinter_1 stderrLinter_1" } };
    const LintCombine::StringVector stdoutData{ "stdoutLinter_1" };
    const LintCombine::StringVector stderrData{ "stderrLinter_1" };
    const CWLTestCase::Output output{ /*diagnostics*/{}, stdoutData, stderrData,
                                      /*filesWithContent*/{}, LintCombine::AllLintersSucceeded };
}

namespace TestCWL::L1WFR0 {
    const CWLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToFile" +
          getScriptExtension() + " 0 " + pathToTempDir + "MockFile_1 fileLinter_1" } };
    const std::vector< CWLTestCase::FileData > filesWithContent{
        { pathToTempDir + "MockFile_1", { "fileLinter_1" } } };
    const CWLTestCase::Output output{ /*diagnostics*/{}, /*stdoutData*/{}, /*stderrData*/{},
                                      filesWithContent, LintCombine::AllLintersSucceeded };
}

namespace TestCWL::L1WSFR0 {
    const CWLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreamsAndToFile" + getScriptExtension() +
          " 0 " + pathToTempDir + "MockFile_1 fileLinter_1 stdoutLinter_1 stderrLinter_1" } };
    const LintCombine::StringVector stdoutData{ "stdoutLinter_1" };
    const LintCombine::StringVector stderrData{ "stderrLinter_1" };
    const std::vector< CWLTestCase::FileData > filesWithContent{
        { pathToTempDir + "MockFile_1", { "fileLinter_1" } } };
    const CWLTestCase::Output output{
        /*diagnostics*/{}, stdoutData, stderrData, filesWithContent, LintCombine::AllLintersSucceeded };
}

namespace TestCWL::L1WSR0_L2WSR0 {
    const CWLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreams" +
          getScriptExtension() + " 0 stdoutLinter_1 stderrLinter_1",
          "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreams" +
          getScriptExtension() + " 0 stdoutLinter_2 stderrLinter_2" } };
    const LintCombine::StringVector stdoutData{ "stdoutLinter_1", "stdoutLinter_2" };
    const LintCombine::StringVector stderrData{ "stderrLinter_1", "stderrLinter_2" };
    const CWLTestCase::Output output{ /*diagnostics*/{}, stdoutData, stderrData,
                                      /*filesWithContent*/{}, LintCombine::AllLintersSucceeded };
}

namespace TestCWL::L1WFR0_L2WFR0 {
    const CWLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToFile" +
          getScriptExtension() + " 0 " + pathToTempDir + "MockFile_1 fileLinter_1",
          "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToFile" +
          getScriptExtension() + " 0 " + pathToTempDir + "MockFile_2 fileLinter_2" } };
    const std::vector< CWLTestCase::FileData > filesWithContent{
        { pathToTempDir + "MockFile_1", { "fileLinter_1" } },
        { pathToTempDir + "MockFile_2", { "fileLinter_2" } } };
    const CWLTestCase::Output output{ /*diagnostics*/{}, /*stdoutData*/{}, /*stderrData*/{},
                                      filesWithContent, LintCombine::AllLintersSucceeded };
}

namespace TestCWL::L1WSFR0_L2WSFR0 {
    const CWLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreamsAndToFile" +
          getScriptExtension() + " 0 " + pathToTempDir + "MockFile_1 fileLinter_1 "
          "stdoutLinter_1 stderrLinter_1",
          "--sub-linter=MockLinterWrapper", getRunnerName( "cmd" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreamsAndToFile" +
          getScriptExtension() + " 0 " + pathToTempDir + "MockFile_2 fileLinter_2 "
          "stdoutLinter_2 stderrLinter_2" } };
    const LintCombine::StringVector stdoutData{ "stdoutLinter_1", "stdoutLinter_2" };
    const LintCombine::StringVector stderrData{ "stderrLinter_1", "stderrLinter_2" };
    const std::vector< CWLTestCase::FileData > filesWithContent{
        { pathToTempDir + "MockFile_1", { "fileLinter_1" } },
        { pathToTempDir + "MockFile_2", { "fileLinter_2" } } };
    const CWLTestCase::Output output{ /*diagnostics*/{}, stdoutData, stderrData,
                                      filesWithContent, LintCombine::AllLintersSucceeded };
}

namespace TestCWL::LintersWorkInParallel {
    const CWLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", getRunnerName( "bash" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_1.sh 0 0.5 mes_1 mes_3",
          "--sub-linter=MockLinterWrapper", getRunnerName( "bash" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockParallelTest_2.sh 0 0.25 mes_2" } };
    const LintCombine::StringVector stdoutData{ "mes_1\nmes_2\nmes_3\n" };
    const CWLTestCase::Output output{ /*diagnostics*/{}, stdoutData, /*stderrData*/{},
                                      /*filesWithContent*/{}, LintCombine::AllLintersSucceeded };
}

namespace TestCWL::OneLinterEETC {
    const CWLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", getRunnerName( "bash" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
          + pathToTempDir + "file_1.txt" }, { pathToTempDir + "file_1.txt" } };
    const CWLTestCase::Output output{ /*diagnostics=*/{}, /*stdoutData=*/{}, /*stderrData=*/{},
                                      /*filesWithContent=*/{}, LintCombine::AllLintersSucceeded };
}

namespace TestCWL::TwoLinterEETC {
    const CWLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", getRunnerName( "bash" ) +
          CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
          + pathToTempDir + "file_1.txt", "--sub-linter=MockLinterWrapper",
          getRunnerName( "bash" ) + CURRENT_SOURCE_DIR "mockPrograms/mockSleepAndRemoveFile.sh 0.3 "
          + pathToTempDir + "file_2.txt" },
          { pathToTempDir + "file_1.txt", pathToTempDir + "file_2.txt" } };
    const CWLTestCase::Output output{ /*diagnostics=*/{}, /*stdoutData=*/{}, /*stderrData=*/{},
                                      /*filesWithContent=*/{}, LintCombine::AllLintersSucceeded };
}

const CWLTestCase CWLTestCaseData[] = {
    /*0 */ { TestCWL::L1Terminate::input, TestCWL::L1Terminate::output },
    /*1 */ { TestCWL::L1Terminate_L2WSFR0::input, TestCWL::L1Terminate_L2WSFR0::output },
    /*2 */ { TestCWL::L1Terminate_L2Terminate::input, TestCWL::L1Terminate_L2Terminate::output },
    /*3 */ { TestCWL::L1WSR1::input, TestCWL::L1WSR1::output },
    /*4 */ { TestCWL::L1WSR1_L2WSFR0::input, TestCWL::L1WSR1_L2WSFR0::output },
    /*5 */ { TestCWL::L1R1_L2R1::input, TestCWL::L1R1_L2R1::output },
    /*6 */ { TestCWL::L1WSR0::input, TestCWL::L1WSR0::output },
    /*7 */ { TestCWL::L1WFR0::input, TestCWL::L1WFR0::output },
    /*8 */ { TestCWL::L1WSFR0::input, TestCWL::L1WSFR0::output },
    /*9 */ { TestCWL::L1WSR0_L2WSR0::input, TestCWL::L1WSR0_L2WSR0::output },
    /*10*/ { TestCWL::L1WFR0_L2WFR0::input, TestCWL::L1WFR0_L2WFR0::output },
    /*11*/ { TestCWL::L1WSFR0_L2WSFR0::input, TestCWL::L1WSFR0_L2WSFR0::output },
    /*12*/ { TestCWL::LintersWorkInParallel::input, TestCWL::LintersWorkInParallel::output },
    /*13*/ { TestCWL::OneLinterEETC::input, TestCWL::OneLinterEETC::output },
    /*14*/ { TestCWL::TwoLinterEETC::input, TestCWL::TwoLinterEETC::output },
};

BOOST_DATA_TEST_CASE( TestCallAndWaitLinter, CWLTestCaseData, sample ) {
    for( const auto & fileName : sample.input.fileNamesForLinterEndsEarlyTest ) {
        std::ofstream{ fileName, std::ios_base::trunc };
    }
    const auto & correctResult = static_cast< CWLTestCase::Output >( sample.output );
    LintCombine::LinterCombine combine(
        sample.input.cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    std::string stdoutData;
    std::string stderrData;
    int combineReturnCode;
    { // Capture only LinterCombine's output
        const StreamCapture stdoutCapture( std::cout );
        const StreamCapture stderrCapture( std::cerr );
        combine.callLinter();
        combineReturnCode = combine.waitLinter();
        stdoutData = stdoutCapture.getBufferData();
        stderrData = stderrCapture.getBufferData();
    }
    BOOST_CHECK( combineReturnCode == correctResult.returnCode );
    for( const auto & correctStdoutStr : correctResult.stdoutData ) {
        BOOST_CHECK( stdoutData.find( correctStdoutStr ) != std::string::npos );
    }
    for( const auto & correctStderrStr : correctResult.stderrData ) {
        BOOST_CHECK( stderrData.find( correctStderrStr ) != std::string::npos );
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
    checkThatDiagnosticsAreEqual( combine.diagnostics(), correctResult.diagnostics );
    for( const auto & fileName : sample.input.fileNamesForLinterEndsEarlyTest ) {
        BOOST_CHECK( !std::filesystem::exists( fileName ) );
        std::filesystem::remove( fileName );
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
                const pairStrStrVec & filesToCompareVal )
            : diagnostics( diagnosticsVal ), callTotals( callTotalsVal ),
              filesToCompare( filesToCompareVal ) {}
        std::vector< LintCombine::Diagnostic > diagnostics;
        LintCombine::CallTotals callTotals;
        pairStrStrVec filesToCompare;
    };

    struct Input {
        Input( const LintCombine::StringVector & cmdLineVal,
               LintCombine::LinterFactoryBase & factoryVal =
               LintCombine::MocksLinterFactory::getInstance() )
            : cmdLine( cmdLineVal ), factory( factoryVal ) {}
        LintCombine::StringVector cmdLine;
        LintCombine::LinterFactoryBase & factory;
    };

    UYTestCase( const Input & inputVal, const Output & outputVal )
        : input( inputVal ), output( outputVal ) {}

    Input input;
    Output output;
};

std::ostream & operator<<( std::ostream & os, UYTestCase ) {
    return os;
}

namespace TestUY::L1YPDNE {
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Warning,
          "defaultLinter's YAML-file doesn't exist", "LinterBase", 1, 0 },
        { LintCombine::Level::Warning, "Updating 1 YAML-files failed", "LintCombine", 1, 0 } };
    const UYTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", "defaultLinter", "NonexistentFile" } };
    const UYTestCase::Output output{
        diagnostics, { /*successNum=*/0, /*failNum=*/1 }, /*filesToCompare=*/{}};
}

namespace TestUY::L1YPEmpty {
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Warning, "defaultLinter's YAML-file doesn't exist", "LinterBase", 1, 0 },
        { LintCombine::Level::Warning, "Updating 1 YAML-files failed", "LintCombine", 1, 0 } };
    const UYTestCase::Input input{ { "--result-yaml=" + pathToTempDir + "mockG",
                                     "--sub-linter=MockLinterWrapper", "defaultLinter", "" } };
    const UYTestCase::Output output{
        diagnostics, { /*successNum=*/0, /*failNum=*/1 }, /*filesToCompare=*/{}};
}

namespace TestUY::L1YPEmpty_L2YPE {
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Warning, "defaultLinter's YAML-file doesn't exist", "LinterBase", 1, 0 },
        { LintCombine::Level::Warning, "Updating 1 YAML-files failed", "LintCombine", 1, 0 } };
    const pairStrStrVec filesToCompare{
        std::make_pair( CURRENT_SOURCE_DIR "/yamlFiles/linterFile_2.yaml",
                        pathToTempDir + "linterFile_2.yaml" ) };
    const UYTestCase::Input input{ { "--result-yaml=" + pathToTempDir + "mockG",
                                     "--sub-linter=MockLinterWrapper", "defaultLinter", "",
                                     "--sub-linter=MockLinterWrapper", "defaultLinter",
                                     pathToTempDir + "/linterFile_2.yaml" } };
    const UYTestCase::Output output{
        diagnostics, { /*successNum=*/1, /*failNum=*/1 }, filesToCompare };
}

namespace TestUY::TwoLintersYPEmpty {
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Warning, "defaultLinter's YAML-file doesn't exist", "LinterBase", 1, 0 },
        { LintCombine::Level::Warning, "defaultLinter's YAML-file doesn't exist", "LinterBase", 1, 0 },
        { LintCombine::Level::Warning, "Updating 2 YAML-files failed", "LintCombine", 1, 0 } };
    const UYTestCase::Input input{ { "--result-yaml=" + pathToTempDir + "mockG",
                                     "--sub-linter=MockLinterWrapper", "defaultLinter", "",
                                     "--sub-linter=MockLinterWrapper", "defaultLinter", "" } };
    const UYTestCase::Output output{
        diagnostics, { /*successNum=*/0, /*failNum=*/2 }, /*filesToCompare=*/{} };
}

namespace TestUY::L1YPE_L2YPDNE {
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Warning, "defaultLinter's YAML-file doesn't exist", "LinterBase", 1, 0 },
        { LintCombine::Level::Warning, "Updating 1 YAML-files failed", "LintCombine", 1, 0 } };
    const pairStrStrVec filesToCompare{
        std::make_pair( CURRENT_SOURCE_DIR "/yamlFiles/linterFile_2.yaml",
                        pathToTempDir + "linterFile_2.yaml" ) };
    const UYTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "mockG",
          "--sub-linter=MockLinterWrapper", "defaultLinter", "NonexistentFile",
          "--sub-linter=MockLinterWrapper", "defaultLinter", pathToTempDir + "linterFile_2.yaml" } };
    const UYTestCase::Output output{ diagnostics, { /*successNum=*/1, /*failNum=*/1 }, filesToCompare };
}

namespace TestUY::TwoLintersYPDNE {
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Warning, "defaultLinter's YAML-file doesn't exist", "LinterBase", 1, 0 },
        { LintCombine::Level::Warning, "defaultLinter's YAML-file doesn't exist", "LinterBase", 1, 0 },
        { LintCombine::Level::Warning, "Updating 2 YAML-files failed", "LintCombine", 1, 0 } };
    const UYTestCase::Input input{ { "--result-yaml=" + pathToTempDir + "mockG",
                                     "--sub-linter=MockLinterWrapper", "defaultLinter", "NonexistentFile",
                                     "--sub-linter=MockLinterWrapper", "defaultLinter", "NonexistentFile" } };
    const UYTestCase::Output output{ diagnostics, { /*successNum=*/0, /*failNum=*/2 }, /*filesToCompare=*/{} };
}

namespace TestUY::L1_YPE {
    const pairStrStrVec filesToCompare{
        std::make_pair( CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml",
                        pathToTempDir + "linterFile_1.yaml" ) };
    const UYTestCase::Input input{ { "--result-yaml=" + pathToTempDir + "mockG",
                                     "--sub-linter=MockLinterWrapper", "defaultLinter",
                                     pathToTempDir + "linterFile_1.yaml" } };
    const UYTestCase::Output output{ /*diagnostics=*/{}, { /*successNum=*/1, /*failNum=*/0 }, filesToCompare };
}

namespace TestUY::L1YPE_L2YPE {
    const pairStrStrVec filesToCompare{
        std::make_pair( CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml",
                        pathToTempDir + "linterFile_1.yaml" ),
        std::make_pair( CURRENT_SOURCE_DIR "yamlFiles/linterFile_2.yaml",
                        pathToTempDir + "linterFile_2.yaml" ) };
    const UYTestCase::Input input{ { "--result-yaml=" + pathToTempDir + "mockG",
                                     "--sub-linter=MockLinterWrapper", "defaultLinter",
                                     pathToTempDir + "linterFile_1.yaml",
                                     "--sub-linter=MockLinterWrapper", "defaultLinter",
                                     pathToTempDir + "linterFile_2.yaml" } };
    const UYTestCase::Output output{
        /*diagnostics=*/{}, { /*successNum=*/2, /*failNum=*/0 }, filesToCompare };
}

namespace TestUY::clangTidyUpdateYaml {
    const pairStrStrVec filesToCompare{
        std::make_pair( pathToTempDir + "linterFile_1.yaml",
                        CURRENT_SOURCE_DIR "/yamlFiles/linterFile_1_expected.yaml" ) };
    const UYTestCase::Input input{ { "--result-yaml=" + pathToTempDir + "mockG",
                                     "--sub-linter=clang-tidy", "--export-fixes=" +
                                     pathToTempDir + "linterFile_1.yaml" },
                                     LintCombine::UsualLinterFactory::getInstance() };
    const UYTestCase::Output output{
        /*diagnostics=*/{}, { /*successNum=*/1, /*failNum=*/0 }, filesToCompare };
}

namespace TestUY::clazyUpdateYaml {
    const pairStrStrVec filesToCompare{
        std::make_pair( pathToTempDir + "linterFile_2.yaml",
                        CURRENT_SOURCE_DIR "/yamlFiles/linterFile_2_expected.yaml" ) };
    const UYTestCase::Input input{ { "--result-yaml=" + pathToTempDir + "mockG",
                                     "--sub-linter=clazy", "--export-fixes=" +
                                     pathToTempDir + "linterFile_2.yaml" },
                                     LintCombine::UsualLinterFactory::getInstance() };
    const UYTestCase::Output output{
        /*diagnostics=*/{}, { /*successNum=*/1, /*failNum=*/0 }, filesToCompare };
}

const UYTestCase UYTestCaseData[] = {
    /*0 */ { TestUY::L1YPDNE::input, TestUY::L1YPDNE::output },
    /*1 */ { TestUY::L1YPEmpty::input, TestUY::L1YPEmpty::output },
    /*2 */ { TestUY::L1YPEmpty_L2YPE::input, TestUY::L1YPEmpty_L2YPE::output },
    /*3 */ { TestUY::TwoLintersYPEmpty::input, TestUY::TwoLintersYPEmpty::output },
    /*4 */ { TestUY::L1YPE_L2YPDNE::input, TestUY::L1YPE_L2YPDNE::output },
    /*5 */ { TestUY::TwoLintersYPDNE::input, TestUY::TwoLintersYPDNE::output },
    /*6 */ { TestUY::L1_YPE::input, TestUY::L1_YPE::output },
    /*7 */ { TestUY::L1YPE_L2YPE::input, TestUY::L1YPE_L2YPE::output },
    /*8 */ { TestUY::clangTidyUpdateYaml::input, TestUY::clangTidyUpdateYaml::output },
    /*9 */ { TestUY::clazyUpdateYaml::input, TestUY::clazyUpdateYaml::output },
};

BOOST_DATA_TEST_CASE( TestUpdatedYaml, UYTestCaseData, sample ) {
    const auto & correctResult = static_cast< UYTestCase::Output >( sample.output );
    LintCombine::LinterCombine combine( sample.input.cmdLine, sample.input.factory );
    deleteTempDirWithYamls( pathToTempDir );
    copyRequiredYamlFilesIntoTempDir( pathToTempDir );
    const auto callTotals = combine.updateYaml();
    BOOST_CHECK( callTotals.successNum == correctResult.callTotals.successNum );
    BOOST_CHECK( callTotals.failNum == correctResult.callTotals.failNum );
    checkThatDiagnosticsAreEqual( combine.diagnostics(), correctResult.diagnostics );
    for( const auto & [lhs, rhs] : correctResult.filesToCompare ) {
        std::ifstream yamlFileAct( lhs );
        std::ifstream yamlFileExp( rhs );
        std::istream_iterator< char > fileIterAct( yamlFileAct ), endAct;
        std::istream_iterator< char > fileIterExp( yamlFileExp ), endExp;
        BOOST_CHECK_EQUAL_COLLECTIONS( fileIterAct, endAct, fileIterExp, endExp );
    }
    deleteTempDirWithYamls( pathToTempDir );
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

const std::string pathToTempDir = generatePathToTempDir();

// MY means MergeYaml
struct MYTestCase {
    struct Output {
        Output( const std::vector< LintCombine::Diagnostic > & diagnosticsVal,
                const pairStrStrVec & filesToCompareVal,
                const std::string & pathToCombinedYamlVal )
            : diagnostics( diagnosticsVal ), filesToCompare( filesToCompareVal ),
              pathToCombinedYaml( pathToCombinedYamlVal ) {}
        std::vector< LintCombine::Diagnostic > diagnostics;
        pairStrStrVec filesToCompare;
        std::string pathToCombinedYaml;
    };

    struct Input {
        Input( const LintCombine::StringVector & cmdLineVal )
            : cmdLine( cmdLineVal ) {}
        LintCombine::StringVector cmdLine;
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
    const MYTestCase::Input input{
        { "--result-yaml=" CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
          "--sub-linter=clang-tidy", "--export-fixes=" CURRENT_SOURCE_DIR "NonexistentFile" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Warning,
          "Linter's YAML-file path \"" CURRENT_SOURCE_DIR "NonexistentFile\" doesn't exist",
          "LintCombine", 1, 0 },
        { LintCombine::Level::Error, "Combined YAML-file isn't created", "LintCombine", 1, 0 } };
    const MYTestCase::Output output{
        diagnostics, /*filesToCompare=*/{}, /*resultPathToCombinedYaml=*/{} };
}

namespace TestMY::L1YPE {
    const MYTestCase::Input input{
        { "--result-yaml=" CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
          "--sub-linter=clang-tidy",
          "--export-fixes=" + pathToTempDir + "linterFile_1.yaml" } };
    std::string resultPathToCombinedYaml = CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml";
    const pairStrStrVec filesToCompare{
        std::make_pair( CURRENT_SOURCE_DIR "/yamlFiles/combinedResult.yaml",
                        pathToTempDir + "linterFile_1.yaml" ) };
    const MYTestCase::Output output{ /*diagnostics=*/{}, filesToCompare, resultPathToCombinedYaml };
}

namespace TestMY::L1YPE_L2YPDNE {
    const MYTestCase::Input input{
        { "--result-yaml=" CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
          "--sub-linter=clang-tidy",
          "--export-fixes=" + pathToTempDir + "linterFile_1.yaml",
          "--sub-linter=clang-tidy",
          "--export-fixes=" CURRENT_SOURCE_DIR "NonexistentFile"} };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Warning,
          "Linter's YAML-file path \"" CURRENT_SOURCE_DIR "NonexistentFile\" doesn't exist",
          "LintCombine", 1, 0 } };
    std::string resultPathToCombinedYaml = CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml";
    const pairStrStrVec filesToCompare{
        std::make_pair( CURRENT_SOURCE_DIR "/yamlFiles/combinedResult.yaml",
                        pathToTempDir + "linterFile_1.yaml" ) };
    const MYTestCase::Output output{ diagnostics, filesToCompare, resultPathToCombinedYaml };
}

namespace TestMY::TwoLintersYPDNE {
    const MYTestCase::Input input{ {
        "--result-yaml=" CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
        "--sub-linter=clang-tidy",
        "--export-fixes=" CURRENT_SOURCE_DIR "NonexistentFile",
        "--sub-linter=clang-tidy",
        "--export-fixes=" CURRENT_SOURCE_DIR "NonexistentFile"} };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Warning,
          "Linter's YAML-file path \"" CURRENT_SOURCE_DIR "NonexistentFile\" doesn't exist",
          "LintCombine", 1, 0 },
        { LintCombine::Level::Warning,
          "Linter's YAML-file path \"" CURRENT_SOURCE_DIR "NonexistentFile\" doesn't exist",
          "LintCombine", 1, 0 },
        { LintCombine::Level::Error, "Combined YAML-file isn't created", "LintCombine", 1, 0 } };
    const MYTestCase::Output output{ diagnostics, /*filesToCompare=*/{}, /*resultPathToCombinedYaml=*/{} };
}

namespace TestMY::TwoLintersYPE {
    const MYTestCase::Input input{
        { "--result-yaml=" CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml",
          "--sub-linter=clang-tidy",
          "--export-fixes=" + pathToTempDir + "linterFile_1.yaml",
          "--sub-linter=clang-tidy",
          "--export-fixes=" + pathToTempDir + "linterFile_2.yaml"} };
    std::string resultPathToCombinedYaml = CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml";
    const pairStrStrVec filesToCompare{
        std::make_pair( CURRENT_SOURCE_DIR "/yamlFiles/combinedResult.yaml",
                        CURRENT_SOURCE_DIR "/yamlFiles/combinedResult_expected.yaml" ) };
    const MYTestCase::Output output{
        /*diagnostics=*/{}, filesToCompare, resultPathToCombinedYaml };
}

const MYTestCase MYTestCaseData[] = {
    /*0 */ { TestMY::L1YPDNE::input, TestMY::L1YPDNE::output },
    /*1 */ { TestMY::L1YPE::input, TestMY::L1YPE::output },
    /*2 */ { TestMY::L1YPE_L2YPDNE::input, TestMY::L1YPE_L2YPDNE::output },
    /*3 */ { TestMY::TwoLintersYPDNE::input, TestMY::TwoLintersYPDNE::output },
    /*4 */ { TestMY::TwoLintersYPE::input, TestMY::TwoLintersYPE::output },
};

BOOST_DATA_TEST_CASE( TestMergeYaml, MYTestCaseData, sample ) {
    const auto & correctResult = static_cast< MYTestCase::Output >( sample.output );
    LintCombine::LinterCombine combine( sample.input.cmdLine );
    deleteTempDirWithYamls( pathToTempDir );
    copyRequiredYamlFilesIntoTempDir( pathToTempDir );
    BOOST_CHECK( combine.getYamlPath() == correctResult.pathToCombinedYaml );
    if( correctResult.pathToCombinedYaml.empty() ) {
        BOOST_REQUIRE( !std::filesystem::exists( correctResult.pathToCombinedYaml ) );
    }
    else {
        BOOST_REQUIRE( std::filesystem::exists( correctResult.pathToCombinedYaml ) );
    }
    checkThatDiagnosticsAreEqual( combine.diagnostics(), correctResult.diagnostics );
    for( const auto & [lhs, rhs] : correctResult.filesToCompare ) {
        std::ifstream yamlFileAct( lhs );
        std::ifstream yamlFileExp( rhs );
        std::istream_iterator< char > fileIterAct( yamlFileAct ), endAct;
        std::istream_iterator< char > fileIterExp( yamlFileExp ), endExp;
        BOOST_CHECK_EQUAL_COLLECTIONS( fileIterAct, endAct, fileIterExp, endExp );
    }
    if( !correctResult.pathToCombinedYaml.empty() ) {
        std::filesystem::remove( correctResult.pathToCombinedYaml );
    }
    deleteTempDirWithYamls( pathToTempDir );
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

constexpr bool YAML_MUST_CONTAIN_DOCLINK = true;
constexpr bool YAML_MUST_NOT_CONTAIN_DOCLINK = false;
constexpr bool TOLERANT_TO_LINTER_EXIT_CODE = true;
constexpr bool INTOLERANT_TO_LINTER_EXIT_CODE = false;

// PCL means PrepareCommandLine
struct PCLTestCase {
    struct Input {
        Input( const LintCombine::StringVector & cmdLineVal ) : cmdLine( cmdLineVal ) {}
        LintCombine::StringVector cmdLine;
    };

    struct Output {
        Output( const std::vector< LintCombine::Diagnostic > & diagnosticsVal,
                const LintCombine::StringVector & resultCmdLineVal,
                const std::optional< bool > & mayYamlFileContainDocLinkVal,
                const std::optional< bool > & linterExitCodeTolerantVal )
            : diagnostics( diagnosticsVal ), resultCmdLine( resultCmdLineVal ),
              mayYamlFileContainDocLink( mayYamlFileContainDocLinkVal ),
              linterExitCodeTolerant( linterExitCodeTolerantVal ) {}
        std::vector< LintCombine::Diagnostic > diagnostics;
        LintCombine::StringVector resultCmdLine;
        std::optional< bool > mayYamlFileContainDocLink;
        std::optional< bool > linterExitCodeTolerant;
    };

    PCLTestCase( const Input & inputVal, const Output & outputVal )
        : input( inputVal ), output( outputVal ) {}
    Input input;
    Output output;
};

std::ostream & operator<<( std::ostream & os, PCLTestCase ) {
    return os;
}

void setUpIdeBehaviour( const std::string & ideName,
                        bool & yamlContainsDocLink, bool & linterExitCodeTolerant ) {
    if( ideName == "CLion" ) {
        yamlContainsDocLink = YAML_MUST_NOT_CONTAIN_DOCLINK;
        if constexpr( BOOST_OS_WINDOWS ) {
            linterExitCodeTolerant = INTOLERANT_TO_LINTER_EXIT_CODE;
        }
        if constexpr( BOOST_OS_LINUX ) {
            linterExitCodeTolerant = TOLERANT_TO_LINTER_EXIT_CODE;
        }
    }
    if( ideName == "ReSharper" ) {
        yamlContainsDocLink = YAML_MUST_CONTAIN_DOCLINK;
        linterExitCodeTolerant = INTOLERANT_TO_LINTER_EXIT_CODE;
    }
}

namespace TestPCL::EmptyCmdLine {
    const PCLTestCase::Input input{ {} };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error, "Command Line is empty", "FactoryPreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::Verbatim_LintersDNE {
    const PCLTestCase::Input input{ { "--param=value", "-p=val", "--param", "val" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Info, "Options were passed verbatim", "VerbatimPreparer", 1, 0 },
        { LintCombine::Level::Error, "No linters specified. Use --sub-linter, see --help.",
          "VerbatimPreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::Verbatim_L1IN {
    const PCLTestCase::Input input{
        { "--sub-linter=Incorrect", "--param=value", "-p=val", "--param", "val" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Info, "Options were passed verbatim", "VerbatimPreparer", 1, 0 },
        { LintCombine::Level::Error, "Unknown linter name: \"Incorrect\"", "VerbatimPreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::Verbatim_L1IN_L2IN {
    const PCLTestCase::Input input{ { "--sub-linter=Incorrect_1", "--sub-linter=Incorrect_2",
                                      "--param=value", "-p=val", "--param", "val" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Info, "Options were passed verbatim", "VerbatimPreparer", 1, 0 },
        { LintCombine::Level::Error, "Unknown linter name: \"Incorrect_1\"", "VerbatimPreparer", 1, 0 },
        { LintCombine::Level::Error, "Unknown linter name: \"Incorrect_2\"", "VerbatimPreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::Verbatim_L1CN {
    const PCLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "file.yaml",
          "--sub-linter=clazy", "--param=value", "-p=val", "--param", "val" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Info, "Options were passed verbatim", "VerbatimPreparer", 1, 0 } };
    const LintCombine::StringVector resultCmdLine{ input.cmdLine };
    const PCLTestCase::Output output{ diagnostics, resultCmdLine,
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::Verbatim_L1CN_L2CN {
    const PCLTestCase::Input input{
        { "--result-yaml=" + pathToTempDir + "file.yaml", "--sub-linter=clazy",
          "--sub-linter=clang-tidy", "--param=value", "-p=val", "--param", "val" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Info, "Options were passed verbatim", "VerbatimPreparer", 1, 0 } };
    const LintCombine::StringVector resultCmdLine{
        "--result-yaml=" + pathToTempDir + "file.yaml", "--sub-linter=clazy",
        "--sub-linter=clang-tidy", "--param=value", "-p=val", "--param", "val" };
    const PCLTestCase::Output output{ diagnostics, resultCmdLine,
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::Verbatim_CombinedYamlDNE {
    const PCLTestCase::Input input{
        { "--sub-linter=clazy", "--param=value", "-p=val", "--param", "val" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Info, "Options were passed verbatim", "VerbatimPreparer", 1, 0 },
        { LintCombine::Level::Error, "Path to combined YAML-file is not set", "VerbatimPreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::Verbatim_CombinedYamlIN {
    const PCLTestCase::Input input{
        { "--result-yaml=" INCORRECT_PATH, "--sub-linter=clazy",
          "--param=value", "-p=val", "--param", "val" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Info, "Options were passed verbatim", "VerbatimPreparer", 1, 0 },
        { LintCombine::Level::Error,
          "Combined YAML-file \"" INCORRECT_PATH "\" is not creatable", "VerbatimPreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::UnsupportedIDE {
    const PCLTestCase::Input input{ { "--ide-profile=shasharper" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{ { LintCombine::Level::Error,
          "\"shasharper\" is not a supported IDE profile", "FactoryPreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      std::nullopt, std::nullopt }; // IdeTraitsFactory will return nullptr
}

namespace TestPCL::SpecifiedTwice {
    const PCLTestCase::Input input{
        { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase", "-p=pathToCompilationDataBase" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "option '--p' cannot be specified more than once", "BasePreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::GenYamlPathEmpty {
    const PCLTestCase::Input input{ { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error, "Path to yaml-file is empty.", "BasePreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::CompilationDBEmpty {
    const PCLTestCase::Input input{ { "--ide-profile=ReSharper", "--export-fixes=pathToResultYaml" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error, "Path to compilation database is empty.", "BasePreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::RequiredOptionsE {
    PCLTestCase::Input input( const std::string & ideName ) {
        return { { "--ide-profile=" + ideName,
                   "-p=pathToCompilationDataBase", "--export-fixes=pathToResultYaml" } };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::StringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            { LintCombine::Level::Info, "All linters are used", "BasePreparer", 1, 0 } };
        auto yamlContainsDocLink = false;
        auto linterExitCodeTolerant = false;
        setUpIdeBehaviour( ideName, yamlContainsDocLink, linterExitCodeTolerant );
        return { diagnostics, resultCmdLine, yamlContainsDocLink, linterExitCodeTolerant };
    }
}

namespace TestPCL::CTOptionsPassed {
    PCLTestCase::Input input( const std::string & ideName ) {
        return { { "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
                   "--export-fixes=pathToResultYaml", "--param_1", "@param_2" } };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::StringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--param_1", "@param_2",
            "--sub-linter=clazy", "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase" PATH_SEP "diagnosticsClazy.yaml" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            { LintCombine::Level::Info, "All linters are used", "BasePreparer", 1, 0 } };
        auto yamlContainsDocLink = false;
        auto linterExitCodeTolerant = false;
        setUpIdeBehaviour( ideName, yamlContainsDocLink, linterExitCodeTolerant );
        return { diagnostics, resultCmdLine, yamlContainsDocLink, linterExitCodeTolerant };
    }
}

namespace TestPCL::FilesPassed {
    PCLTestCase::Input input( const std::string & ideName ) {
        return { { "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
            "--export-fixes=pathToResultYaml", "file_1.cpp", "file_2.cpp" } };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::StringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "file_1.cpp", "file_2.cpp",
            "--sub-linter=clazy", "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase" PATH_SEP "diagnosticsClazy.yaml",
            "file_1.cpp", "file_2.cpp" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            { LintCombine::Level::Info, "All linters are used", "BasePreparer", 1, 0 } };
        auto yamlContainsDocLink = false;
        auto linterExitCodeTolerant = false;
        setUpIdeBehaviour( ideName, yamlContainsDocLink, linterExitCodeTolerant );
        return { diagnostics, resultCmdLine, yamlContainsDocLink, linterExitCodeTolerant };
    }
}

namespace TestPCL::ReSharperHeaderFilterPassed {
    const PCLTestCase::Input input{ {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--header-filter=file.cpp" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Info, "All linters are used", "BasePreparer", 1, 0 } };
    const LintCombine::StringVector resultCmdLine{
        "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClangTidy.yaml", "--header-filter=file.cpp",
        "--sub-linter=clazy", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClazy.yaml", "--header-filter=file.cpp" };
    const PCLTestCase::Output output{ diagnostics, resultCmdLine,
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::CLChecksEmptyAES {
    const PCLTestCase::Input input{
        { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
          "--export-fixes=pathToResultYaml", "--clazy-checks=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "the argument for option '--clazy-checks' should follow immediately after the equal sign",
          "FactoryPreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::ClangExtraArgsEmptyAES {
    const PCLTestCase::Input input{
        { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
          "--export-fixes=pathToResultYaml", "--clang-extra-args=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "the argument for option '--clang-extra-args' should follow immediately after the equal sign",
          "FactoryPreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::ParamsEmptyAES {
    const PCLTestCase::Input input{
        { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
          "--export-fixes=pathToResultYaml", "--clazy-checks=", "--clang-extra-args=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "the argument for option '--clazy-checks' should follow immediately after the equal sign",
          "FactoryPreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::CLChecksEmptyASP {
    PCLTestCase::Input input( const std::string & ideName ) {
        return { { "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
                   "--export-fixes=pathToResultYaml", "--clazy-checks" } };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::StringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            { LintCombine::Level::Info, "All linters are used", "BasePreparer", 1, 0 },
            { LintCombine::Level::Warning,
              "Parameter \"clazy-checks\" was set but the parameter's value was not set. "
              "The parameter will be ignored.", "BasePreparer", 63, 75 } };
        auto yamlContainsDocLink = false;
        auto linterExitCodeTolerant = false;
        setUpIdeBehaviour( ideName, yamlContainsDocLink, linterExitCodeTolerant );
        return { diagnostics, resultCmdLine, yamlContainsDocLink, linterExitCodeTolerant };
    }
}

namespace TestPCL::ClangXArgsEmptyASP {
    PCLTestCase::Input input( const std::string & ideName ) {
        return { { "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
                   "--export-fixes=pathToResultYaml", "--clang-extra-args" } };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::StringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            { LintCombine::Level::Info, "All linters are used", "BasePreparer", 1, 0 },
            { LintCombine::Level::Warning,
              "Parameter \"clang-extra-args\" was set but the parameter's value was not set. "
              "The parameter will be ignored.", "BasePreparer", 63, 79 } };
        auto yamlContainsDocLink = false;
        auto linterExitCodeTolerant = false;
        setUpIdeBehaviour( ideName, yamlContainsDocLink, linterExitCodeTolerant );
        return { diagnostics, resultCmdLine, yamlContainsDocLink, linterExitCodeTolerant };
    }
}

namespace TestPCL::ParamsEmptyASP {
    PCLTestCase::Input input( const std::string & ideName ) {
        return { { "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
            "--export-fixes=pathToResultYaml", "--clazy-checks", "--clang-extra-args" } };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::StringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            { LintCombine::Level::Info, "All linters are used", "BasePreparer", 1, 0 },
            { LintCombine::Level::Warning,
              "Parameter \"clazy-checks\" was set but the parameter's value was not set. "
              "The parameter will be ignored.", "BasePreparer", 63, 75 },
            { LintCombine::Level::Warning,
              "Parameter \"clang-extra-args\" was set but the parameter's value was not set. "
              "The parameter will be ignored.", "BasePreparer", 78, 94 } };
        auto yamlContainsDocLink = false;
        auto linterExitCodeTolerant = false;
        setUpIdeBehaviour( ideName, yamlContainsDocLink, linterExitCodeTolerant );
        return { diagnostics, resultCmdLine, yamlContainsDocLink, linterExitCodeTolerant };
    }
}

namespace TestPCL::CLChecksE {
    PCLTestCase::Input input( const std::string & ideName ) {
        return { { "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
            "--export-fixes=pathToResultYaml", "--clazy-checks", "level0,level1" } };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::StringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml", "--checks=level0,level1" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            { LintCombine::Level::Info, "All linters are used", "BasePreparer", 1, 0 } };
        auto yamlContainsDocLink = false;
        auto linterExitCodeTolerant = false;
        setUpIdeBehaviour( ideName, yamlContainsDocLink, linterExitCodeTolerant );
        return { diagnostics, resultCmdLine, yamlContainsDocLink, linterExitCodeTolerant };
    }
}

namespace TestPCL::ClangXArgsE {
    PCLTestCase::Input input( const std::string & ideName ) {
        return { { "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
                   "--export-fixes=pathToResultYaml", "--clang-extra-args=arg_1 arg_2 " } };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::StringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml", "--extra-arg=arg_1", "--extra-arg=arg_2" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            { LintCombine::Level::Info, "All linters are used", "BasePreparer", 1, 0 } };
        auto yamlContainsDocLink = false;
        auto linterExitCodeTolerant = false;
        setUpIdeBehaviour( ideName, yamlContainsDocLink, linterExitCodeTolerant );
        return { diagnostics, resultCmdLine, yamlContainsDocLink, linterExitCodeTolerant };
    }
}

namespace TestPCL::ParamsEAES {
    PCLTestCase::Input input( const std::string & ideName ) {
        return { { "--ide-profile=" + ideName,
                   "-p=pathToCompilationDataBase", "--export-fixes=pathToResultYaml",
                   "--clazy-checks=level0,level1", "--clang-extra-args=arg_1 arg_2 " } };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::StringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml", "--checks=level0,level1",
            "--extra-arg=arg_1", "--extra-arg=arg_2" };

        const std::vector< LintCombine::Diagnostic > diagnostics{
            { LintCombine::Level::Info, "All linters are used", "BasePreparer", 1, 0 } };
        auto yamlContainsDocLink = false;
        auto linterExitCodeTolerant = false;
        setUpIdeBehaviour( ideName, yamlContainsDocLink, linterExitCodeTolerant );
        return { diagnostics, resultCmdLine, yamlContainsDocLink, linterExitCodeTolerant };
    }
}

namespace TestPCL::LinterEmptyAES {
    const PCLTestCase::Input input{
        { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
          "--export-fixes=pathToResultYaml", "--sub-linter=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "the argument for option '--sub-linter' should follow immediately after the equal sign",
          "FactoryPreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::L1FIN_L2EAES {
    const PCLTestCase::Input input{
        { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
          "--export-fixes=pathToResultYaml", "--sub-linter=IncorrectName_1", "--sub-linter=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "the argument for option '--sub-linter' should follow immediately after the equal sign",
          "FactoryPreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::L1EmptyAES_L2IN {
    const PCLTestCase::Input input{
        { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
          "--export-fixes=pathToResultYaml", "--sub-linter=", "--sub-linter=IncorrectName_1" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "the argument for option '--sub-linter' should follow immediately after the equal sign",
          "FactoryPreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::L1EmptyAES_L2EmptyAES {
    const PCLTestCase::Input input{
        { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
          "--export-fixes=pathToResultYaml", "--sub-linter=", "--sub-linter=" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "the argument for option '--sub-linter' should follow immediately after the equal sign",
          "FactoryPreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::L1Empty {
    const PCLTestCase::Input input{
        { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
          "--export-fixes=pathToResultYaml", "--sub-linter" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "the required argument for option '--sub-linter' is missing", "BasePreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };

}

namespace TestPCL::L1IN_L2EmptyASP {
    const PCLTestCase::Input input{
        { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
          "--export-fixes=pathToResultYaml", "--sub-linter", "IncorrectName_1", "--sub-linter" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Error,
          "the required argument for option '--sub-linter' is missing", "BasePreparer", 1, 0 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::L1EmptyASP_L2IN {
    const PCLTestCase::Input input{
        { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
          "--export-fixes=pathToResultYaml", "--sub-linter", "--sub-linter", "IncorrectName_1" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Info, "All linters are used", "BasePreparer", 1, 0 },
        { LintCombine::Level::Error, "Unknown linter name \"--sub-linter\"", "BasePreparer", 74, 86 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::L1EmptyASP_L2EmptyASP {
    const PCLTestCase::Input input{
        { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
          "--export-fixes=pathToResultYaml", "--sub-linter", "--sub-linter" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Info, "All linters are used", "BasePreparer", 1, 0 },
        { LintCombine::Level::Error, "Unknown linter name \"--sub-linter\"", "BasePreparer", 74, 86 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::L1IN {
    const PCLTestCase::Input input{
        { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
          "--export-fixes=pathToResultYaml", "--sub-linter=IncorrectName_1" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Info, "All linters are used", "BasePreparer", 1, 0 },
        { LintCombine::Level::Error, "Unknown linter name \"IncorrectName_1\"", "BasePreparer", 74, 89 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::L1IN_L2IN {
    const PCLTestCase::Input input{
        { "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
          "--export-fixes=pathToResultYaml",
          "--sub-linter=IncorrectName_1", "--sub-linter=IncorrectName_2" } };
    const std::vector< LintCombine::Diagnostic > diagnostics{
        { LintCombine::Level::Info, "All linters are used", "BasePreparer", 1, 0 },
        { LintCombine::Level::Error, "Unknown linter name \"IncorrectName_1\"", "BasePreparer", 74, 89 },
        { LintCombine::Level::Error, "Unknown linter name \"IncorrectName_2\"", "BasePreparer", 103, 118 } };
    const PCLTestCase::Output output{ diagnostics, /*resultCmdLine=*/{},
                                      YAML_MUST_CONTAIN_DOCLINK, INTOLERANT_TO_LINTER_EXIT_CODE };
}

namespace TestPCL::LinterIsCT {
    PCLTestCase::Input input( const std::string & ideName ) {
        return { { "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
                 "--export-fixes=pathToResultYaml", "--sub-linter=clang-tidy" } };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::StringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml" };

        auto yamlContainsDocLink = false;
        auto linterExitCodeTolerant = false;
        setUpIdeBehaviour( ideName, yamlContainsDocLink, linterExitCodeTolerant );
        return { /*diagnostics=*/{}, resultCmdLine, yamlContainsDocLink, linterExitCodeTolerant };
    }
}

namespace TestPCL::LinterIsCL {
    PCLTestCase::Input input( const std::string & ideName ) {
        return { { "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
            "--export-fixes=pathToResultYaml", "--sub-linter=clazy" } };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::StringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml" };

        auto yamlContainsDocLink = false;
        auto linterExitCodeTolerant = false;
        setUpIdeBehaviour( ideName, yamlContainsDocLink, linterExitCodeTolerant );
        return { /*diagnostics=*/{}, resultCmdLine, yamlContainsDocLink, linterExitCodeTolerant };
    }
}

namespace TestPCL::AllLintersAES {
    PCLTestCase::Input input( const std::string & ideName ) {
        return { { "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
                   "--export-fixes=pathToResultYaml", "--sub-linter=clang-tidy", "--sub-linter=clazy" } };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::StringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml" };

        auto yamlContainsDocLink = false;
        auto linterExitCodeTolerant = false;
        setUpIdeBehaviour( ideName, yamlContainsDocLink, linterExitCodeTolerant );
        return { /*diagnostics=*/{}, resultCmdLine, yamlContainsDocLink, linterExitCodeTolerant };
    }
}

namespace TestPCL::AllLintersASP {
    PCLTestCase::Input input( const std::string & ideName ) {
        return { { "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
            "--export-fixes=pathToResultYaml",
            "--sub-linter", "clang-tidy", "--sub-linter", "clazy" } };
    }

    PCLTestCase::Output output( const std::string & ideName ) {
        const LintCombine::StringVector resultCmdLine{
            "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
            "-p=pathToCompilationDataBase", "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml" };

        auto yamlContainsDocLink = false;
        auto linterExitCodeTolerant = false;
        setUpIdeBehaviour( ideName, yamlContainsDocLink, linterExitCodeTolerant );
        return { /*diagnostics=*/{}, resultCmdLine, yamlContainsDocLink, linterExitCodeTolerant };
    }
}

const PCLTestCase PCLTestCaseData[] = {
    /*0 */ { TestPCL::EmptyCmdLine::input, TestPCL::EmptyCmdLine::output },
    /*1 */ { TestPCL::Verbatim_LintersDNE::input, TestPCL::Verbatim_LintersDNE::output },
    /*2 */ { TestPCL::Verbatim_L1IN::input, TestPCL::Verbatim_L1IN::output },
    /*3 */ { TestPCL::Verbatim_L1IN_L2IN::input, TestPCL::Verbatim_L1IN_L2IN::output },
    /*4 */ { TestPCL::Verbatim_L1CN::input, TestPCL::Verbatim_L1CN::output },
    /*5 */ { TestPCL::Verbatim_L1CN_L2CN::input, TestPCL::Verbatim_L1CN_L2CN::output },
    /*6 */ { TestPCL::Verbatim_CombinedYamlDNE::input, TestPCL::Verbatim_CombinedYamlDNE::output },
    /*7 */ { TestPCL::Verbatim_CombinedYamlIN::input, TestPCL::Verbatim_CombinedYamlIN::output },
    /*8 */ { TestPCL::UnsupportedIDE::input, TestPCL::UnsupportedIDE::output },
    /*9 */ { TestPCL::SpecifiedTwice::input, TestPCL::SpecifiedTwice::output },
    /*10*/ { TestPCL::CompilationDBEmpty::input, TestPCL::CompilationDBEmpty::output },
    /*11*/ { TestPCL::GenYamlPathEmpty::input, TestPCL::GenYamlPathEmpty::output },
    /*12*/ { TestPCL::RequiredOptionsE::input( "ReSharper" ), TestPCL::RequiredOptionsE::output( "ReSharper" ) },
    /*13*/ { TestPCL::RequiredOptionsE::input( "CLion" ), TestPCL::RequiredOptionsE::output( "CLion" ) },
    /*14*/ { TestPCL::CTOptionsPassed::input( "ReSharper" ), TestPCL::CTOptionsPassed::output( "ReSharper" ) },
    /*15*/ { TestPCL::CTOptionsPassed::input( "CLion" ), TestPCL::CTOptionsPassed::output( "CLion" ) },
    /*16*/ { TestPCL::FilesPassed::input( "ReSharper" ), TestPCL::FilesPassed::output( "ReSharper" ) },
    /*17*/ { TestPCL::FilesPassed::input( "CLion" ), TestPCL::FilesPassed::output( "CLion" ) },
    /*18*/ { TestPCL::ReSharperHeaderFilterPassed::input, TestPCL::ReSharperHeaderFilterPassed::output },
    /*19*/ { TestPCL::CLChecksEmptyAES::input, TestPCL::CLChecksEmptyAES::output },
    /*20*/ { TestPCL::ClangExtraArgsEmptyAES::input, TestPCL::ClangExtraArgsEmptyAES::output },
    /*21*/ { TestPCL::ParamsEmptyAES::input, TestPCL::ParamsEmptyAES::output },
    /*22*/ { TestPCL::CLChecksEmptyASP::input( "ReSharper" ), TestPCL::CLChecksEmptyASP::output( "ReSharper" ) },
    /*23*/ { TestPCL::CLChecksEmptyASP::input( "CLion" ), TestPCL::CLChecksEmptyASP::output( "CLion" ) },
    /*24*/ { TestPCL::ClangXArgsEmptyASP::input( "ReSharper" ), TestPCL::ClangXArgsEmptyASP::output( "ReSharper" ) },
    /*25*/ { TestPCL::ClangXArgsEmptyASP::input( "CLion" ), TestPCL::ClangXArgsEmptyASP::output( "CLion" ) },
    /*26*/ { TestPCL::ParamsEmptyASP::input( "ReSharper" ), TestPCL::ParamsEmptyASP::output( "ReSharper" ) },
    /*27*/ { TestPCL::ParamsEmptyASP::input( "CLion" ), TestPCL::ParamsEmptyASP::output( "CLion" ) },
    /*28*/ { TestPCL::CLChecksE::input( "ReSharper" ), TestPCL::CLChecksE::output( "ReSharper" ) },
    /*29*/ { TestPCL::CLChecksE::input( "CLion" ), TestPCL::CLChecksE::output( "CLion" ) },
    /*30*/ { TestPCL::ClangXArgsE::input( "ReSharper" ), TestPCL::ClangXArgsE::output( "ReSharper" ) },
    /*31*/ { TestPCL::ClangXArgsE::input( "CLion" ), TestPCL::ClangXArgsE::output( "CLion" ) },
    /*32*/ { TestPCL::ParamsEAES::input( "ReSharper" ), TestPCL::ParamsEAES::output( "ReSharper" ) },
    /*33*/ { TestPCL::ParamsEAES::input( "CLion" ), TestPCL::ParamsEAES::output( "CLion" ) },
    /*34*/ { TestPCL::LinterEmptyAES::input, TestPCL::LinterEmptyAES::output },
    /*35*/ { TestPCL::L1FIN_L2EAES::input, TestPCL::L1FIN_L2EAES::output },
    /*36*/ { TestPCL::L1EmptyAES_L2IN::input, TestPCL::L1EmptyAES_L2IN::output },
    /*37*/ { TestPCL::L1EmptyAES_L2EmptyAES::input, TestPCL::L1EmptyAES_L2EmptyAES::output },
    /*38*/ { TestPCL::L1Empty::input, TestPCL::L1Empty::output },
    /*39*/ { TestPCL::L1IN_L2EmptyASP::input, TestPCL::L1IN_L2EmptyASP::output },
    /*40*/ { TestPCL::L1EmptyASP_L2IN::input, TestPCL::L1EmptyASP_L2IN::output },
    /*41*/ { TestPCL::L1EmptyASP_L2EmptyASP::input, TestPCL::L1EmptyASP_L2EmptyASP::output },
    /*42*/ { TestPCL::L1IN::input, TestPCL::L1IN::output },
    /*43*/ { TestPCL::L1IN_L2IN::input, TestPCL::L1IN_L2IN::output },
    /*44*/ { TestPCL::LinterIsCT::input( "ReSharper" ), TestPCL::LinterIsCT::output( "ReSharper" ) },
    /*45*/ { TestPCL::LinterIsCT::input( "CLion" ), TestPCL::LinterIsCT::output( "CLion" ) },
    /*46*/ { TestPCL::LinterIsCL::input( "ReSharper" ), TestPCL::LinterIsCL::output( "ReSharper" ) },
    /*47*/ { TestPCL::LinterIsCL::input( "CLion" ), TestPCL::LinterIsCL::output( "CLion" ) },
    /*48*/ { TestPCL::AllLintersAES::input( "ReSharper" ), TestPCL::AllLintersAES::output( "ReSharper" ) },
    /*49*/ { TestPCL::AllLintersAES::input( "CLion" ), TestPCL::AllLintersAES::output( "CLion" ) },
    /*50*/ { TestPCL::AllLintersASP::input( "ReSharper" ), TestPCL::AllLintersASP::output( "ReSharper" ) },
    /*51*/ { TestPCL::AllLintersASP::input( "CLion" ), TestPCL::AllLintersASP::output( "CLion" ) },
};

BOOST_DATA_TEST_CASE( TestPrepareCmdLine, PCLTestCaseData, sample ) {
    const auto & correctResult = static_cast< PCLTestCase::Output >( sample.output );
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto inputCmdLine = sample.input.cmdLine;
    auto prepareCmdLine = ideTraitsFactory.getPrepareInputsInstance( inputCmdLine );
    if( correctResult.mayYamlFileContainDocLink.has_value() ) {
        BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->mayYamlFileContainDocLink() ==
                     correctResult.mayYamlFileContainDocLink );
    }
    if( correctResult.linterExitCodeTolerant.has_value() ) {
        BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->isLinterExitCodeTolerant() ==
                     correctResult.linterExitCodeTolerant );
    }
    auto preparerResult = prepareCmdLine->transformCmdLine( inputCmdLine );
    BOOST_CHECK_EQUAL_COLLECTIONS( preparerResult.begin(), preparerResult.end(),
                                   correctResult.resultCmdLine.begin(), correctResult.resultCmdLine.end() );
    checkThatDiagnosticsAreEqual( prepareCmdLine->diagnostics(), correctResult.diagnostics );
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestSpecifyTargetArch )

void checkTargetArch( const std::string & macrosDir,
                      const std::string & targetTriple = std::string() ) {
    if constexpr( !BOOST_OS_WINDOWS ) { return; }
    LintCombine::StringVector cmdLine = {
        "--ide-profile=CLion", "-p=" CURRENT_SOURCE_DIR "CLionTestsMacros/"
        + macrosDir, "--export-fixes=pathToResultYaml"
    };
    LintCombine::StringVector result;
    if( !targetTriple.empty() ) {
        const std::string extraArg =
            "--extra-arg-before=\"--target=" + targetTriple + "\"";
        result = {
           "--result-yaml=pathToResultYaml",  "--sub-linter=clang-tidy",
           "-p=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir,
           "--export-fixes=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir +
           PATH_SEP "diagnosticsClangTidy.yaml", extraArg,
            "--sub-linter=clazy", "-p=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir,
           "--export-fixes=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir +
           PATH_SEP "diagnosticsClazy.yaml", extraArg
        };
    }
    else {
        result = {
           "--result-yaml=pathToResultYaml", "--sub-linter=clang-tidy",
           "-p=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir,
           "--export-fixes=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir +
           PATH_SEP "diagnosticsClangTidy.yaml", "--sub-linter=clazy",
           "-p=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir,
           "--export-fixes=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir +
           PATH_SEP "diagnosticsClazy.yaml"
        };
    }

    auto prepareCmdLine = LintCombine::IdeTraitsFactory().getPrepareInputsInstance( cmdLine );
    auto preparerResult = prepareCmdLine->transformCmdLine( cmdLine );
    BOOST_CHECK_EQUAL_COLLECTIONS( preparerResult.begin(), preparerResult.end(),
                                   result.begin(), result.end() );
}

BOOST_AUTO_TEST_CASE( MacrosFileDoesNotExist ) {
    const std::string macrosDir = "emptyDir";
    checkTargetArch( macrosDir, /*targetTriple=*/{} );
}

BOOST_AUTO_TEST_CASE( EmptyMacrosFile ) {
    const std::string macrosDir = "emptyFileInsideDir";
    checkTargetArch( macrosDir, /*targetTriple=*/{} );
}

BOOST_AUTO_TEST_CASE( ArchMacrosDontSpecified ) {
    const std::string macrosDir = "archMacrosDoNotSpecified";
    checkTargetArch( macrosDir, /*targetTriple=*/{} );
}

BOOST_AUTO_TEST_CASE( SeveralDifferentArchSpecified ) {
    const std::string macrosDir = "severalDifferentArchSpecified";
    checkTargetArch( macrosDir, /*targetTriple=*/{} );
}

BOOST_AUTO_TEST_CASE( MacrosX86 ) {
    const std::string macrosDir = "x86";
    const std::string targetTriple = "i386-pc-win32";
    checkTargetArch( macrosDir, targetTriple );
}

BOOST_AUTO_TEST_CASE( MacrosX64 ) {
    const std::string macrosDir = "x64";
    const std::string targetTriple = "x86_64-pc-win32";
    checkTargetArch( macrosDir, targetTriple );
}

BOOST_AUTO_TEST_CASE( MacrosArm ) {
    const std::string macrosDir = "arm";
    const std::string targetTriple = "arm-pc-win32";
    checkTargetArch( macrosDir, targetTriple );
}

BOOST_AUTO_TEST_CASE( MacrosArm64 ) {
    const std::string macrosDir = "arm64";
    const std::string targetTriple = "arm64-pc-win32";
    checkTargetArch( macrosDir, targetTriple );
}

BOOST_AUTO_TEST_SUITE_END()
