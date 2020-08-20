#define BOOST_TEST_MODULE CppLintCombineTest

#include "../../src/LinterCombine.h"
#include "../../src/LinterBase.h"
#include "../../src/IdeTraitsFactory.h"

#include <boost/test/included/unit_test.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <filesystem>
#include <memory>

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

    std::string getBufferData() const {
        return buffer.str();
    }

private:
    std::ostream * stream;
    std::ostringstream buffer;
    std::streambuf * old;
};

namespace LintCombine {
    struct MockLinterWrapper final : LinterBase {
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

void compareContainers( const LintCombine::stringVector & lhs,
                        const LintCombine::stringVector & rhs ) {
    BOOST_REQUIRE( lhs.size() == rhs.size() );
    for( size_t i = 0; i < lhs.size(); ++i ) {
        BOOST_CHECK( lhs[i] == rhs[i] );
    }
}

BOOST_AUTO_TEST_SUITE( TestLinterCombineConstructor )

BOOST_AUTO_TEST_CASE( emptyCmdLine ) {
    const LintCombine::stringVector cmdLine = {};
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.numLinters() == 0 );
    BOOST_CHECK( combine.getIsErrorOccur() == true );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Command Line is empty" );
}

BOOST_AUTO_TEST_CASE( LintersNotExists ) {
    const LintCombine::stringVector cmdLine = {
        "--param_1=value_1", "--param_2=value_2" };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.numLinters() == 0 );
    BOOST_CHECK( combine.getIsErrorOccur() == true );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "No one linter parsed" );
}

BOOST_AUTO_TEST_CASE( OneLinterNotExists ) {
    const LintCombine::stringVector cmdLine = {
        "--sub-linter=NotExistentLinter" };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.numLinters() == 0 );
    BOOST_CHECK( combine.getIsErrorOccur() == true );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text ==
                 "Unknown linter name: \"NotExistentLinter\"" );
}

BOOST_AUTO_TEST_CASE( FirstLinterNotExistsSecondExists ) {
    const LintCombine::stringVector cmdLine = {
        "--sub-linter=NotExistentLinter", "--sub-linter=clazy" };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.numLinters() == 0 );
    BOOST_CHECK( combine.getIsErrorOccur() == true );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text ==
                 "Unknown linter name: \"NotExistentLinter\"" );
}

BOOST_AUTO_TEST_CASE( TwoLintersNotExist ) {
    const LintCombine::stringVector cmdLine = {
        "--sub-linter=NotExistentLinter_1",
        "--sub-linter=NotExistentLinter_2" };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.numLinters() == 0 );
    BOOST_REQUIRE( combine.diagnostics().size() == 2 );
    BOOST_CHECK( combine.getIsErrorOccur() == true );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text ==
                 "Unknown linter name: \"NotExistentLinter_1\"" );
    const auto diagnostic_1 = combine.diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text ==
                 "Unknown linter name: \"NotExistentLinter_2\"" );
}

BOOST_AUTO_TEST_CASE( LintersValueEmptyAfterEqualSign ) {
    const LintCombine::stringVector cmdLine = { "--sub-linter=" };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.numLinters() == 0 );
    BOOST_REQUIRE( combine.diagnostics().size() == 1 );
    BOOST_CHECK( combine.getIsErrorOccur() == true );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the argument for option "
                 "'--sub-linter' should follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( LintersValueEmptyAfterSpace ) {
    const LintCombine::stringVector cmdLine = { "--sub-linter" };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.numLinters() == 0 );
    BOOST_REQUIRE( combine.diagnostics().size() == 1 );
    BOOST_CHECK( combine.getIsErrorOccur() == true );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the required argument for "
                 "option '--sub-linter' is missing" );
}

BOOST_AUTO_TEST_CASE( TwoLintersValuesEmptyAfterEqualSign ) {
    const LintCombine::stringVector cmdLine = {
        "--sub-linter=", "--sub-linter=" };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.numLinters() == 0 );
    BOOST_REQUIRE( combine.diagnostics().size() == 1 );
    BOOST_CHECK( combine.getIsErrorOccur() == true );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the argument for option "
                 "'--sub-linter' should follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( TwoLintersValuesEmptyAfterSpace ) {
    const LintCombine::stringVector cmdLine = {
        "--sub-linter", "--sub-linter" };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.numLinters() == 0 );
    BOOST_REQUIRE( combine.diagnostics().size() == 1 );
    BOOST_CHECK( combine.getIsErrorOccur() == true );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "No one linter parsed" );
}

BOOST_AUTO_TEST_CASE( GeneralYamlPathValueEmptyAfterEqualSign ) {
    const LintCombine::stringVector cmdLine = {
        "--result-yaml=", "--sub-linter=clazy" };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.numLinters() == 0 );
    BOOST_CHECK( combine.numLinters() == 0 );
    BOOST_REQUIRE( combine.diagnostics().size() == 1 );
    BOOST_CHECK( combine.getIsErrorOccur() == true );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the argument for option "
                 "'--result-yaml' should follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( GeneralYamlPathValueEmptyAfterEqualSpace ) {
    const LintCombine::stringVector cmdLine = {
        "--result-yaml", "--sub-linter=clazy" };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.numLinters() == 1 );
    const auto & linter_0 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 0 ) );
    BOOST_CHECK( linter_0->getName() == "clazy-standalone" );
    BOOST_CHECK( linter_0->getOptions().empty() );
    BOOST_CHECK( linter_0->getYamlPath() ==
                 CURRENT_BINARY_DIR + linter_0->getName() + "-Diagnostics.yaml" );
    BOOST_REQUIRE( combine.diagnostics().size() == 2 );
    BOOST_CHECK( combine.getIsErrorOccur() == false );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Incorrect general YAML file name: "
                 "\"--sub-linter=clazy\"" );
    const auto diagnostic_1 = combine.diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_1.origin == "Combine" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text ==
                 "General YAML file path changed to "
                 CURRENT_BINARY_DIR "LintersDiagnostics.yaml" );
}

BOOST_AUTO_TEST_CASE( GeneralYamlPathValueIncorrect ) {
    const LintCombine::stringVector cmdLine = {
        "--result-yaml=\\\\", "--sub-linter=clazy" };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.numLinters() == 1 );
    const auto & linter_0 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 0 ) );
    BOOST_CHECK( linter_0->getName() == "clazy-standalone" );
    BOOST_CHECK( linter_0->getOptions().empty() );
    BOOST_CHECK( linter_0->getYamlPath() ==
                 CURRENT_BINARY_DIR + linter_0->getName() + "-Diagnostics.yaml" );
    BOOST_REQUIRE( combine.diagnostics().size() == 2 );
    BOOST_CHECK( combine.getIsErrorOccur() == false );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text ==
                 "Incorrect general YAML file name: \"\\\\\"" );
    const auto diagnostic_1 = combine.diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_1.origin == "Combine" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text ==
                 "General YAML file path changed to "
                 CURRENT_BINARY_DIR "LintersDiagnostics.yaml" );
}

BOOST_AUTO_TEST_CASE( LinterYamlPathValueEmptyAfterEqualSign ) {
    const LintCombine::stringVector cmdLine = {
        "--sub-linter=clazy", "--export-fixes=" };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.numLinters() == 0 );
    BOOST_REQUIRE( combine.diagnostics().size() == 1 );
    BOOST_CHECK( combine.getIsErrorOccur() == true );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the argument for option "
                 "'--export-fixes' should follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( LinterYamlPathValueEmptyAfterEqualSpace ) {
    const LintCombine::stringVector cmdLine = {
        "--sub-linter=clazy", "--export-fixes" };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.numLinters() == 1 );
    const auto & linter_0 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 0 ) );
    BOOST_CHECK( linter_0->getName() == "clazy-standalone" );
    BOOST_CHECK( linter_0->getOptions().empty() );
    BOOST_CHECK( linter_0->getYamlPath() ==
                 CURRENT_BINARY_DIR + linter_0->getName() + "-Diagnostics.yaml" );
    BOOST_REQUIRE( combine.diagnostics().size() == 2 );//
    BOOST_CHECK( combine.getIsErrorOccur() == false );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_0.origin == "clazy-standalone" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the required argument for option "
                 "'--export-fixes' is missing" );
    const auto diagnostic_1 = combine.diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_1.origin == "clazy-standalone" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text ==
                 "General YAML file path changed to "
                 CURRENT_BINARY_DIR "clazy-standalone-Diagnostics.yaml" );
}

BOOST_AUTO_TEST_CASE( LinterYamlPathValueIncorrect ) {
    const LintCombine::stringVector cmdLine = {
        "--sub-linter=clazy", "--export-fixes=\\\\" };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.numLinters() == 1 );
    const auto & linter_0 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 0 ) );
    BOOST_CHECK( linter_0->getName() == "clazy-standalone" );
    BOOST_CHECK( linter_0->getOptions().empty() );
    BOOST_CHECK( linter_0->getYamlPath() ==
                 CURRENT_BINARY_DIR + linter_0->getName() + "-Diagnostics.yaml" );
    BOOST_REQUIRE( combine.diagnostics().size() == 2 );//
    BOOST_CHECK( combine.getIsErrorOccur() == false );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_0.origin == "clazy-standalone" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text ==
                 "Incorrect linter's YAML file name: \"\\\\\"" );
    const auto diagnostic_1 = combine.diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_1.origin == "clazy-standalone" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text ==
                 "General YAML file path changed to "
                 CURRENT_BINARY_DIR "clazy-standalone-Diagnostics.yaml" );
}

BOOST_AUTO_TEST_CASE( ClazyExists ) {
    const LintCombine::stringVector cmdLine = {
        "--result-yaml=SomeFile.yaml", "--sub-linter=clazy" };
    const LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.getIsErrorOccur() == false );
    BOOST_REQUIRE( combine.numLinters() == 1 );
    const auto & linter_0 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 0 ) );
    BOOST_CHECK( linter_0->getName() == "clazy-standalone" );
    BOOST_CHECK( linter_0->getOptions().empty() );
    BOOST_CHECK( linter_0->getYamlPath() ==
                 CURRENT_BINARY_DIR + linter_0->getName() + "-Diagnostics.yaml" );
}

BOOST_AUTO_TEST_CASE( ClangTidyExists ) {
    const LintCombine::stringVector cmdLine = {
        "--result-yaml", "SomeFile.yaml", "--sub-linter=clang-tidy" };
    const LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.getIsErrorOccur() == false );
    BOOST_REQUIRE( combine.numLinters() == 1 );
    const auto & linter_0 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 0 ) );
    BOOST_CHECK( linter_0->getName() == "clang-tidy" );
    BOOST_CHECK( linter_0->getOptions().empty() );
    BOOST_CHECK( linter_0->getYamlPath() ==
                 CURRENT_BINARY_DIR + linter_0->getName() + "-Diagnostics.yaml" );
}

BOOST_AUTO_TEST_CASE( ClazyExistsAndHasOptions ) {
    const LintCombine::stringVector cmdLine = {
        "--sub-linter=clazy", "lintParam_1", "lintParam_2" };
    const LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.getIsErrorOccur() == false );
    BOOST_CHECK( combine.numLinters() == 1 );
    const auto & linter_0 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 0 ) );
    BOOST_CHECK( linter_0->getName() == "clazy-standalone" );
    BOOST_CHECK( linter_0->getOptions() == "lintParam_1 lintParam_2 " );
    BOOST_CHECK( linter_0->getYamlPath() ==
                 CURRENT_BINARY_DIR + linter_0->getName() + "-Diagnostics.yaml" );
}

BOOST_AUTO_TEST_CASE( ClazyExistsAndHasYamlPath ) {
    const LintCombine::stringVector cmdLine = {
        "--sub-linter=clazy",
        "--export-fixes=/path/to/file.yaml" };
    const LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.getIsErrorOccur() == false );
    BOOST_CHECK( combine.numLinters() == 1 );
    const auto & linter_0 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 0 ) );
    BOOST_CHECK( linter_0->getName() == "clazy-standalone" );
    BOOST_CHECK( linter_0->getOptions().empty() );
    BOOST_CHECK( linter_0->getYamlPath() == "/path/to/file.yaml" );
}

BOOST_AUTO_TEST_CASE( ClazyExistsAndHasOptionsAndYamlPath ) {
    const LintCombine::stringVector cmdLine = {
        "--sub-linter=clazy", "--export-fixes=/path/to/file.yaml",
        "lintParam_1", "lintParam_2" };
    const LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.getIsErrorOccur() == false );
    BOOST_CHECK( combine.numLinters() == 1 );
    const auto & linter_0 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 0 ) );
    BOOST_CHECK( linter_0->getName() == "clazy-standalone" );
    BOOST_CHECK( linter_0->getOptions() == "lintParam_1 lintParam_2 " );
    BOOST_CHECK( linter_0->getYamlPath() == "/path/to/file.yaml" );
}

BOOST_AUTO_TEST_CASE( ClangTidyAndClazyExist ) {
    const LintCombine::stringVector cmdLine = {
        "--result-yaml=SomeFile.yaml",
        "--sub-linter=clang-tidy", "--sub-linter=clazy" };
    const LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.getIsErrorOccur() == false );
    BOOST_REQUIRE( combine.numLinters() == 2 );
    const auto & linter_0 =
        std::dynamic_pointer_cast < LintCombine::LinterBase >
        ( combine.linterAt( 0 ) );
    const auto & linter_1 =
        std::dynamic_pointer_cast < LintCombine::LinterBase >
        ( combine.linterAt( 1 ) );
    BOOST_CHECK( linter_0->getName() == "clang-tidy" );
    BOOST_CHECK( linter_0->getOptions().empty() );
    BOOST_CHECK( linter_0->getYamlPath() ==
                 CURRENT_BINARY_DIR + linter_0->getName() + "-Diagnostics.yaml" );
    BOOST_CHECK( linter_1->getName() == "clazy-standalone" );
    BOOST_CHECK( linter_1->getOptions().empty() );
    BOOST_CHECK( linter_1->getYamlPath() ==
                 CURRENT_BINARY_DIR + linter_1->getName() + "-Diagnostics.yaml" );
}

BOOST_AUTO_TEST_CASE( ClangTidyAndClazyExistAndHaveOptions ) {
    const LintCombine::stringVector cmdLine = {
        "--result-yaml=SomeFile.yaml",
        "--sub-linter=clang-tidy", "CTParam_1", "CTParam_2",
        "--sub-linter=clazy", "CSParam_1", "CSParam_1" };
    const LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.getIsErrorOccur() == false );
    BOOST_REQUIRE( combine.numLinters() == 2 );
    const auto & linter_0 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 0 ) );
    const auto & linter_1 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 1 ) );
    BOOST_CHECK( linter_0->getName() == "clang-tidy" );
    BOOST_CHECK( linter_0->getOptions() == "CTParam_1 CTParam_2 " );
    BOOST_CHECK( linter_0->getYamlPath() ==
                 CURRENT_BINARY_DIR + linter_0->getName() + "-Diagnostics.yaml" );
    BOOST_CHECK( linter_1->getName() == "clazy-standalone" );
    BOOST_CHECK( linter_1->getOptions() == "CSParam_1 CSParam_1 " );
    BOOST_CHECK( linter_1->getYamlPath() ==
                 CURRENT_BINARY_DIR + linter_1->getName() + "-Diagnostics.yaml" );
}

BOOST_AUTO_TEST_CASE( ClangTidyAndClazyExistAndHasYamlPath ) {
    const LintCombine::stringVector cmdLine = {
        "--result-yaml=SomeFile.yaml",
        "--sub-linter=clang-tidy", "--export-fixes=path/to/file_1.yaml",
        "--sub-linter=clazy", "--export-fixes=path/to/file_2.yaml" };
    const LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.getIsErrorOccur() == false );
    BOOST_REQUIRE( combine.numLinters() == 2 );
    const auto & linter_0 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 0 ) );
    const auto & linter_1 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 1 ) );
    BOOST_CHECK( linter_0->getName() == "clang-tidy" );
    BOOST_CHECK( linter_0->getOptions().empty() );
    BOOST_CHECK( linter_0->getYamlPath() == "path/to/file_1.yaml" );
    BOOST_CHECK( linter_1->getName() == "clazy-standalone" );
    BOOST_CHECK( linter_1->getOptions().empty() );
    BOOST_CHECK( linter_1->getYamlPath() == "path/to/file_2.yaml" );
}

BOOST_AUTO_TEST_CASE( ClangTidyAndClazyExistAndHasOptionsAndYamlPath ) {
    const LintCombine::stringVector cmdLine = {
        "--result-yaml=SomeFile.yaml",
        "--sub-linter=clang-tidy", "CTParam_1", "CTParam_2",
        "--export-fixes=path/to/file_1.yaml",
        "--sub-linter=clazy", "CSParam_1", "CSParam_2",
        "--export-fixes=path/to/file_2.yaml" };
    const LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.getIsErrorOccur() == false );
    BOOST_REQUIRE( combine.numLinters() == 2 );
    const auto & linter_0 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 0 ) );
    const auto & linter_1 =
        std::dynamic_pointer_cast < LintCombine::LinterBase > (
        combine.linterAt( 1 ) );
    BOOST_CHECK( linter_0->getName() == "clang-tidy" );
    BOOST_CHECK( linter_0->getOptions() == "CTParam_1 CTParam_2 " );
    BOOST_CHECK( linter_0->getYamlPath() == "path/to/file_1.yaml" );
    BOOST_CHECK( linter_1->getName() == "clazy-standalone" );
    BOOST_CHECK( linter_1->getOptions() == "CSParam_1 CSParam_2 " );
    BOOST_CHECK( linter_1->getYamlPath() == "path/to/file_2.yaml" );
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * Tests names abbreviations:
 * L<n> means: <n>-th linter
 * WS means: wtite to streams (stdout, stderr)
 * WF means: write to file
 * WSF means: write to file and to streams (stdout, stderr)
 * R<n> means: return <n>
*/

BOOST_AUTO_TEST_SUITE( TestCallAndWaitLinter )

BOOST_AUTO_TEST_CASE( L1Terminate ) {
    const StreamCapture stdoutCapture( std::cout );
    LintCombine::stringVector cmdLine = {
        "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        cmdLine.emplace_back( "sh.exe " CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStdoutAndTerminate.sh stdoutLinter_1" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        cmdLine.emplace_back( "sh " CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStdoutAndTerminate.sh stdoutLinter_1" );
    }
    LintCombine::LinterCombine combine(
        cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();

    const auto combineReturnCode = combine.waitLinter();
    const auto stdoutData = stdoutCapture.getBufferData();
    BOOST_CHECK( combineReturnCode == 3 );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );

    BOOST_REQUIRE( combine.diagnostics().size() == 1 );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters failed while running" );
}

BOOST_AUTO_TEST_CASE( L1Terminate_L2WSFR0 ) {
    const StreamCapture stdoutCapture( std::cout );
    const StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = {
        "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        cmdLine.emplace_back( "sh.exe " CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStdoutAndTerminate.sh stdoutLinter_1" );
        cmdLine.emplace_back( "--sub-linter=MockLinterWrapper" );
        cmdLine.emplace_back( CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStreamsAndToFile.bat 0 "
                              CURRENT_BINARY_DIR "MockFile_2.yaml fileLinter_2 "
                              "stdoutLinter_2 stderrLinter_2" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        cmdLine.emplace_back( "sh " CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStdoutAndTerminate.sh stdoutLinter_1" );
        cmdLine.emplace_back( "--sub-linter=MockLinterWrapper" );
        cmdLine.emplace_back( "sh " CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStreamsAndToFile.sh 0 "
                              CURRENT_BINARY_DIR "MockFile_2.yaml fileLinter_2 "
                              "stdoutLinter_2 stderrLinter_2" );
    }

    LintCombine::LinterCombine combine(
        cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    const auto linterCombineReturnCode = combine.waitLinter();
    const auto stdoutData = stdoutCapture.getBufferData();
    const auto stderrData = stderrCapture.getBufferData();
    BOOST_CHECK( linterCombineReturnCode == 2 );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_2" ) != std::string::npos );
    BOOST_CHECK( stderrData.find( "stderrLinter_2" ) != std::string::npos );
    BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR "MockFile_2.yaml" ) );
    std::string fileData;
    getline( std::fstream( CURRENT_BINARY_DIR "MockFile_2.yaml" ), fileData );
    BOOST_CHECK( fileData.find( "fileLinter_2" ) != std::string::npos );
    std::filesystem::remove( CURRENT_BINARY_DIR "MockFile_2.yaml" );

    BOOST_REQUIRE( combine.diagnostics().size() == 1 );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Some linters failed while running" );
}

BOOST_AUTO_TEST_CASE( L1Terminate_L2Terminate ) {
    const StreamCapture stdoutCapture( std::cout );
    LintCombine::stringVector cmdLine = {
        "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        cmdLine.emplace_back(
            "sh.exe " CURRENT_SOURCE_DIR
            "mockPrograms/mockWriteToStdoutAndTerminate.sh stdoutLinter_1" );
        cmdLine.emplace_back( "--sub-linter=MockLinterWrapper" );
        cmdLine.emplace_back(
            "sh.exe " CURRENT_SOURCE_DIR
            "mockPrograms/mockWriteToStdoutAndTerminate.sh stdoutLinter_2" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        cmdLine.emplace_back(
            "sh " CURRENT_SOURCE_DIR
            "mockPrograms/mockWriteToStdoutAndTerminate.sh stdoutLinter_1" );
        cmdLine.emplace_back( "--sub-linter=MockLinterWrapper" );
        cmdLine.emplace_back(
            "sh " CURRENT_SOURCE_DIR
            "mockPrograms/mockWriteToStdoutAndTerminate.sh stdoutLinter_2" );
    }

    LintCombine::LinterCombine combine(
        cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    const auto linterCombineReturnCode = combine.waitLinter();
    const auto stdoutData = stdoutCapture.getBufferData();
    BOOST_CHECK( linterCombineReturnCode == 3 );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_2" ) != std::string::npos );

    BOOST_REQUIRE( combine.diagnostics().size() == 1 );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters failed while running" );
}

BOOST_AUTO_TEST_CASE( L1WSR1 ) {
    const StreamCapture stdoutCapture( std::cout );
    const StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        cmdLine.emplace_back(
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToStreams.bat 1 "
            "stdoutLinter_1 stderrLinter_1" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        cmdLine.emplace_back(
            "sh " CURRENT_SOURCE_DIR
            "mockPrograms/mockWriteToStreams.sh 1 "
            "stdoutLinter_1 stderrLinter_1" );
    }

    LintCombine::LinterCombine combine(
        cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    const auto linterCombineReturnCode = combine.waitLinter();
    const auto stdoutData = stdoutCapture.getBufferData();
    const auto stderrData = stderrCapture.getBufferData();
    BOOST_CHECK( linterCombineReturnCode == 3 );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
    BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );

    BOOST_REQUIRE( combine.diagnostics().size() == 1 );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters failed while running" );
}

BOOST_AUTO_TEST_CASE( L1WSR1_L2WSFR0 ) {
    const StreamCapture stdoutCapture( std::cout );
    const StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = {
        "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        cmdLine.emplace_back( CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStreams.bat 1 "
                              "stdoutLinter_1 stderrLinter_1" );
        cmdLine.emplace_back( "--sub-linter=MockLinterWrapper" );
        cmdLine.emplace_back( CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStreamsAndToFile.bat 0 "
                              CURRENT_BINARY_DIR "MockFile_2.yaml fileLinter_2 "
                              "stdoutLinter_2 stderrLinter_2" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        cmdLine.emplace_back( "sh " CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStreams.sh 1 "
                              "stdoutLinter_1 stderrLinter_1" );
        cmdLine.emplace_back( "--sub-linter=MockLinterWrapper" );
        cmdLine.emplace_back( "sh " CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStreamsAndToFile.sh 0 "
                              CURRENT_BINARY_DIR "MockFile_2.yaml fileLinter_2 "
                              "stdoutLinter_2 stderrLinter_2" );
    }

    LintCombine::LinterCombine combine( cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    const auto linterCombineReturnCode = combine.waitLinter();
    const auto stdoutData = stdoutCapture.getBufferData();
    const auto stderrData = stderrCapture.getBufferData();
    BOOST_CHECK( linterCombineReturnCode == 2 );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_2" ) != std::string::npos );
    BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );
    BOOST_CHECK( stderrData.find( "stderrLinter_2" ) != std::string::npos );
    BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR "MockFile_2.yaml" ) );
    std::string fileData;
    getline( std::fstream( CURRENT_BINARY_DIR "MockFile_2.yaml" ), fileData );
    BOOST_CHECK( fileData.find( "fileLinter_2" ) != std::string::npos );
    std::filesystem::remove( CURRENT_BINARY_DIR "MockFile_2.yaml" );

    BOOST_REQUIRE( combine.diagnostics().size() == 1 );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Some linters failed while running" );
}

BOOST_AUTO_TEST_CASE( L1R1_L1R1 ) {
    const StreamCapture stdoutCapture( std::cout );
    const StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = {
        "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        cmdLine.emplace_back( CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStreams.bat 1 "
                              "stdoutLinter_1 stderrLinter_1" );
        cmdLine.emplace_back( "--sub-linter=MockLinterWrapper" );
        cmdLine.emplace_back( CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStreams.bat 1 "
                              "stdoutLinter_2 stderrLinter_2" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        cmdLine.emplace_back( "sh " CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStreams.sh 1 "
                              "stdoutLinter_1 stderrLinter_1" );
        cmdLine.emplace_back( "--sub-linter=MockLinterWrapper" );
        cmdLine.emplace_back( "sh " CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStreams.sh 1 "
                              "stdoutLinter_2 stderrLinter_2" );
    }

    LintCombine::LinterCombine combine(
        cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    const auto linterCombineReturnCode = combine.waitLinter();
    const auto stdoutData = stdoutCapture.getBufferData();
    const auto stderrData = stderrCapture.getBufferData();
    BOOST_CHECK( linterCombineReturnCode == 3 );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_2" ) != std::string::npos );
    BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );
    BOOST_CHECK( stderrData.find( "stderrLinter_2" ) != std::string::npos );

    BOOST_REQUIRE( combine.diagnostics().size() == 1 );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters failed while running" );
}

BOOST_AUTO_TEST_CASE( L1WSR0 ) {
    const StreamCapture stdoutCapture( std::cout );
    const StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        cmdLine.emplace_back( CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStreams.bat 0 "
                              "stdoutLinter_1 stderrLinter_1" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        cmdLine.emplace_back( "sh " CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToStreams.sh 0 "
                              "stdoutLinter_1 stderrLinter_1" );
    }
    LintCombine::LinterCombine combine(
        cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    const auto linterCombineReturnCode = combine.waitLinter();
    const auto stdoutData = stdoutCapture.getBufferData();
    const auto stderrData = stderrCapture.getBufferData();
    BOOST_CHECK( linterCombineReturnCode == 0 );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
    BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );
    BOOST_REQUIRE( combine.diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( L1WFR0 ) {
    const StreamCapture stdoutCapture( std::cout );
    const StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        cmdLine.emplace_back(
            CURRENT_SOURCE_DIR "mockPrograms/mockWriteToFile.bat 0 "
            CURRENT_BINARY_DIR "MockFile_1.yaml fileLinter_1" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        cmdLine.emplace_back( "sh " CURRENT_SOURCE_DIR
                              "mockPrograms/mockWriteToFile.sh 0 "
                              CURRENT_BINARY_DIR "MockFile_1.yaml fileLinter_1" );
    }
    LintCombine::LinterCombine combine(
        cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    BOOST_CHECK( combine.waitLinter() == 0 );
    BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_1.yaml" ) );
    std::string fileData;
    getline( std::fstream( CURRENT_BINARY_DIR"MockFile_1.yaml" ), fileData );
    BOOST_CHECK( fileData.find( "fileLinter_1" ) != std::string::npos );
    std::filesystem::remove( CURRENT_BINARY_DIR "MockFile_1.yaml" );
    BOOST_REQUIRE( combine.diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( L1WSFR0 ) {
    const StreamCapture stdoutCapture( std::cout );
    const StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLineSTL = {
        "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToStreamsAndToFile.bat 0 "
                                     CURRENT_BINARY_DIR "MockFile_1.yaml fileLinter_1 "
                                     "stdoutLinter_1 stderrLinter_1" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToStreamsAndToFile.sh 0 "
                                     CURRENT_BINARY_DIR "MockFile_1.yaml fileLinter_1 "
                                     "stdoutLinter_1 stderrLinter_1" );
    }
    LintCombine::LinterCombine combine(
        commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    const auto linterCombineReturnCode = combine.waitLinter();
    const auto stdoutData = stdoutCapture.getBufferData();
    const auto stderrData = stderrCapture.getBufferData();
    BOOST_CHECK( linterCombineReturnCode == 0 );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
    BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );
    BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_1.yaml" ) );
    std::string fileData;
    getline( std::fstream( CURRENT_BINARY_DIR"MockFile_1.yaml" ), fileData );
    BOOST_CHECK( fileData.find( "fileLinter_1" ) != std::string::npos );
    std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_1.yaml" );
    BOOST_REQUIRE( combine.diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( L1WSR0_L2WSR0 ) {
    const StreamCapture stdoutCapture( std::cout );
    const StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLineSTL = {
        "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToStreams.bat 0 "
                                     "stdoutLinter_1 stderrLinter_1" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToStreams.bat 0 "
                                     "stdoutLinter_2 stderrLinter_2" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToStreams.sh 0 "
                                     "stdoutLinter_1 stderrLinter_1" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToStreams.sh 0 "
                                     "stdoutLinter_2 stderrLinter_2" );
    }
    LintCombine::LinterCombine combine(
        commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    const auto linterCombineReturnCode = combine.waitLinter();
    const auto stdoutData = stdoutCapture.getBufferData();
    const auto stderrData = stderrCapture.getBufferData();
    BOOST_CHECK( linterCombineReturnCode == 0 );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_2" ) != std::string::npos );
    BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );
    BOOST_CHECK( stderrData.find( "stderrLinter_2" ) != std::string::npos );
    BOOST_REQUIRE( combine.diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( L1WFR0_L2WFR0 ) {
    const StreamCapture stdoutCapture( std::cout );
    const StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLineSTL = {
        "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToFile.bat 0 "
                                     CURRENT_BINARY_DIR "MockFile_1.yaml fileLinter_1" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToFile.bat 0 "
                                     CURRENT_BINARY_DIR "MockFile_2.yaml fileLinter_2" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToFile.sh 0 "
                                     CURRENT_BINARY_DIR "MockFile_1.yaml fileLinter_1" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToFile.sh 0 "
                                     CURRENT_BINARY_DIR "MockFile_2.yaml fileLinter_2" );
    }
    LintCombine::LinterCombine combine(
        commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    BOOST_CHECK( combine.waitLinter() == 0 );
    BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR "MockFile_1.yaml" ) );
    std::string fileData_1;
    getline( std::fstream( CURRENT_BINARY_DIR "MockFile_1.yaml" ), fileData_1 );
    BOOST_CHECK( fileData_1.find( "fileLinter_1" ) != std::string::npos );
    std::filesystem::remove( CURRENT_BINARY_DIR "MockFile_1.yaml" );
    BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR "MockFile_2.yaml" ) );
    std::string fileData_2;
    getline( std::fstream( CURRENT_BINARY_DIR "MockFile_2.yaml" ), fileData_2 );
    BOOST_CHECK( fileData_2.find( "fileLinter_2" ) != std::string::npos );
    std::filesystem::remove( CURRENT_BINARY_DIR "MockFile_2.yaml" );
    BOOST_REQUIRE( combine.diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( L1WSR0_L2WFR0 ) {
    const StreamCapture stdoutCapture( std::cout );
    const StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLineSTL = {
        "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToStreams.bat 0 "
                                     "stdoutLinter_1 stderrLinter_1" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToFile.bat 0 "
                                     CURRENT_BINARY_DIR "MockFile_2.yaml fileLinter_2" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToStreams.sh 0 "
                                     "stdoutLinter_1 stderrLinter_1" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToFile.sh 0 "
                                     CURRENT_BINARY_DIR "MockFile_2.yaml fileLinter_2" );
    }

    LintCombine::LinterCombine combine(
        commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    const auto linterCombineReturnCode = combine.waitLinter();
    const auto stdoutData = stdoutCapture.getBufferData();
    const auto stderrData = stderrCapture.getBufferData();
    BOOST_CHECK( linterCombineReturnCode == 0 );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
    BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );
    BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
    std::string fileData;
    getline( std::fstream( CURRENT_BINARY_DIR "MockFile_2.yaml" ), fileData );
    BOOST_CHECK( fileData.find( "fileLinter_2" ) != std::string::npos );
    std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    BOOST_REQUIRE( combine.diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( L1WSFR0_L2WSFR0 ) {
    const StreamCapture stdoutCapture( std::cout );
    const StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector commandLineSTL = {
        "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToStreamsAndToFile.bat 0 "
                                     CURRENT_BINARY_DIR "MockFile_1.yaml fileLinter_1 "
                                     "stdoutLinter_1 stderrLinter_1" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back(
            CURRENT_SOURCE_DIR
            "mockPrograms/mockWriteToStreamsAndToFile.bat 0 "
            CURRENT_BINARY_DIR "MockFile_2.yaml fileLinter_2 "
            "stdoutLinter_2 stderrLinter_2" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToStreamsAndToFile.sh 0 "
                                     CURRENT_BINARY_DIR "MockFile_1.yaml fileLinter_1 "
                                     "stdoutLinter_1 stderrLinter_1" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockWriteToStreamsAndToFile.sh 0 "
                                     CURRENT_BINARY_DIR "MockFile_2.yaml fileLinter_2 "
                                     "stdoutLinter_2 stderrLinter_2" );
    }

    LintCombine::LinterCombine combine(
        commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    const auto linterCombineReturnCode = combine.waitLinter();
    const auto stdoutData = stdoutCapture.getBufferData();
    const auto stderrData = stderrCapture.getBufferData();
    BOOST_CHECK( linterCombineReturnCode == 0 );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_1" ) != std::string::npos );
    BOOST_CHECK( stdoutData.find( "stdoutLinter_2" ) != std::string::npos );
    BOOST_CHECK( stderrData.find( "stderrLinter_1" ) != std::string::npos );
    BOOST_CHECK( stderrData.find( "stderrLinter_2" ) != std::string::npos );
    BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_1.yaml" ) );
    std::string fileData_1;
    getline( std::fstream( CURRENT_BINARY_DIR"MockFile_1.yaml" ), fileData_1 );
    BOOST_CHECK( fileData_1.find( "fileLinter_1" ) != std::string::npos );
    std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_1.yaml" );
    BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"MockFile_2.yaml" ) );
    std::string fileData_2;
    getline( std::fstream( CURRENT_BINARY_DIR"MockFile_2.yaml" ), fileData_2 );
    BOOST_CHECK( fileData_2.find( "fileLinter_2" ) != std::string::npos );
    std::filesystem::remove( CURRENT_BINARY_DIR"MockFile_2.yaml" );
    BOOST_REQUIRE( combine.diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( LintersWorkInParallel ) {
    const StreamCapture stdoutCapture( std::cout );
    LintCombine::stringVector commandLineSTL = {
        "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockParallelTest_1.sh 0 0.5 mes_1 mes_3" );
        commandLineSTL.emplace_back(
            "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( "sh.exe " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockParallelTest_2.sh 0 0.25 mes_2" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockParallelTest_1.sh 0 0.5 mes_1 mes_3" );
        commandLineSTL.emplace_back(
            "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back( "sh " CURRENT_SOURCE_DIR
                                     "mockPrograms/mockParallelTest_2.sh 0 0.25 mes_2" );
    }
    LintCombine::LinterCombine combine(
        commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    const auto linterCombineReturnCode = combine.waitLinter();
    const auto stdoutData = stdoutCapture.getBufferData();
    BOOST_CHECK( linterCombineReturnCode == 0 );
    BOOST_CHECK( stdoutData == "mes_1\nmes_2\nmes_3\n" );
    BOOST_REQUIRE( combine.diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( OneLinterEndsEarlierThanCombine ) {
    std::ofstream( CURRENT_SOURCE_DIR "file_1.txt" );
    LintCombine::stringVector commandLineSTL = { "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back(
            "sh.exe " CURRENT_SOURCE_DIR
            "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
            CURRENT_SOURCE_DIR "file_1.txt" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back(
            "sh " CURRENT_SOURCE_DIR
            "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
            CURRENT_SOURCE_DIR "file_1.txt" );
    }
    LintCombine::LinterCombine combine(
        commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    BOOST_CHECK( combine.waitLinter() == 0 );
    BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR "file_1.txt" ) );
    std::filesystem::remove( CURRENT_SOURCE_DIR "file_1.txt" );
    BOOST_REQUIRE( combine.diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( TwoLinterEndEarlierThanCombine ) {
    std::ofstream( CURRENT_SOURCE_DIR "file_1.txt" );
    std::ofstream( CURRENT_SOURCE_DIR "file_2.txt" );
    LintCombine::stringVector commandLineSTL = {
            "--sub-linter=MockLinterWrapper" };
    if constexpr( BOOST_OS_WINDOWS ) {
        commandLineSTL.emplace_back(
            "sh.exe " CURRENT_SOURCE_DIR
            "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
            CURRENT_SOURCE_DIR"file_1.txt" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back(
            "sh.exe " CURRENT_SOURCE_DIR
            "mockPrograms/mockSleepAndRemoveFile.sh 0.3 "
            CURRENT_SOURCE_DIR"file_2.txt" );
    }
    if constexpr( BOOST_OS_LINUX ) {
        commandLineSTL.emplace_back(
            "sh " CURRENT_SOURCE_DIR
            "mockPrograms/mockSleepAndRemoveFile.sh 0.2 "
            CURRENT_SOURCE_DIR "file_1.txt" );
        commandLineSTL.emplace_back( "--sub-linter=MockLinterWrapper" );
        commandLineSTL.emplace_back(
            "sh " CURRENT_SOURCE_DIR
            "mockPrograms/mockSleepAndRemoveFile.sh 0.3 "
            CURRENT_SOURCE_DIR "file_2.txt" );
    }

    LintCombine::LinterCombine combine(
        commandLineSTL, LintCombine::MocksLinterFactory::getInstance() );
    combine.callLinter();
    BOOST_CHECK( combine.waitLinter() == 0 );
    BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR "file_1.txt" ) );
    std::filesystem::remove( CURRENT_SOURCE_DIR "file_1.txt" );
    BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR "file_2.txt" ) );
    std::filesystem::remove( CURRENT_SOURCE_DIR "file_2.txt" );
    BOOST_REQUIRE( combine.diagnostics().empty() );
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestUpdatedYaml )

BOOST_AUTO_TEST_CASE( LintersYamlPathParamNotExist ) {
    const LintCombine::stringVector cmdLine = {
            "--sub-linter=MockLinterWrapper", "defaultLinter",
            "NotExistentFile" };
    LintCombine::LinterCombine combine(
        cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    const auto callTotals = combine.updateYaml();
    BOOST_CHECK( callTotals.successNum == 0 );
    BOOST_CHECK( callTotals.failNum == 1 );

    BOOST_REQUIRE( combine.diagnostics().size() == 2 );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "LinterBase" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "YAML file path \"NotExistentFile\" doesn't exist" );
    const auto diagnostic_1 = combine.diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.origin == "Combine" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text == "Updating 1 YAML files failed" );
}

BOOST_AUTO_TEST_CASE( EmptyLintersYamlPath ) {
    const LintCombine::stringVector cmdLine = {
            "", "--sub-linter=MockLinterWrapper", "defaultLinter", "" };
    LintCombine::LinterCombine combine(
        cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    const auto callTotals = combine.updateYaml();
    BOOST_CHECK( callTotals.successNum == 0 );
    BOOST_CHECK( callTotals.failNum == 1 );

    BOOST_REQUIRE( combine.diagnostics().size() == 2 );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "LinterBase" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "YAML file path \"\" doesn't exist" );
    const auto diagnostic_1 = combine.diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.origin == "Combine" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text == "Updating 1 YAML files failed" );
}

BOOST_AUTO_TEST_CASE( FirstsYamlPathValueEmptySecondYamlPathExists ) {
    const LintCombine::stringVector cmdLine = {
            "--sub-linter=MockLinterWrapper", "defaultLinter", "",
            "--sub-linter=MockLinterWrapper", "defaultLinter",
            CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" };
    LintCombine::LinterCombine combine(
        cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    const auto callTotals = combine.updateYaml();
    BOOST_CHECK( callTotals.successNum == 1 );
    BOOST_CHECK( callTotals.failNum == 1 );
    std::ifstream yamlFile_2( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" );
    std::ifstream yamlFile_2_save( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2_save.yaml" );
    std::istream_iterator < char > fileIter_2( yamlFile_2 ), end_2;
    std::istream_iterator < char > fileIter_2_save( yamlFile_2_save ), end_2_save;
    BOOST_CHECK_EQUAL_COLLECTIONS( fileIter_2, end_2, fileIter_2_save, end_2_save );

    BOOST_REQUIRE( combine.diagnostics().size() == 2 );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "LinterBase" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "YAML file path \"\" doesn't exist" );
    const auto diagnostic_1 = combine.diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.origin == "Combine" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text == "Updating 1 YAML files failed" );
}

BOOST_AUTO_TEST_CASE( TwoLintersHaveEmptyYamlPathValue ) {
    const LintCombine::stringVector cmdLine = {
            "--sub-linter=MockLinterWrapper", "defaultLinter", "",
            "--sub-linter=MockLinterWrapper", "defaultLinter", "" };
    LintCombine::LinterCombine combine(
        cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    const auto callTotals = combine.updateYaml();
    BOOST_CHECK( callTotals.successNum == 0 );
    BOOST_CHECK( callTotals.failNum == 2 );

    BOOST_REQUIRE( combine.diagnostics().size() == 3 );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "LinterBase" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "YAML file path \"\" doesn't exist" );
    const auto diagnostic_1 = combine.diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.origin == "LinterBase" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text == "YAML file path \"\" doesn't exist" );
    const auto diagnostic_2 = combine.diagnostics()[2];
    BOOST_CHECK( diagnostic_2.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_2.origin == "Combine" );
    BOOST_CHECK( diagnostic_2.firstPos == 1 );
    BOOST_CHECK( diagnostic_2.lastPos == 0 );
    BOOST_CHECK( diagnostic_2.text == "Updating 2 YAML files failed" );
}

BOOST_AUTO_TEST_CASE( FirstsYamlPathNowExistsSecondYamlPathExists ) {
    const LintCombine::stringVector cmdLine = {
            "--sub-linter=MockLinterWrapper", "defaultLinter", "NotExistentFile",
            "--sub-linter=MockLinterWrapper", "defaultLinter",
            CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" };
    LintCombine::LinterCombine combine(
        cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    const auto callTotals = combine.updateYaml();
    BOOST_CHECK( callTotals.successNum == 1 );
    BOOST_CHECK( callTotals.failNum == 1 );
    std::ifstream yamlFile_2( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" );
    std::ifstream yamlFile_2_save( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2_save.yaml" );
    std::istream_iterator < char > fileIter_2( yamlFile_2 ), end_2;
    std::istream_iterator < char > fileIter_2_save( yamlFile_2_save ), end_2_save;
    BOOST_CHECK_EQUAL_COLLECTIONS( fileIter_2, end_2, fileIter_2_save, end_2_save );

    BOOST_REQUIRE( combine.diagnostics().size() == 2 );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "LinterBase" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "YAML file path \"NotExistentFile\" doesn't exist" );
    const auto diagnostic_1 = combine.diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.origin == "Combine" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text == "Updating 1 YAML files failed" );
}

BOOST_AUTO_TEST_CASE( TwoLintersYamlPathValuesNotExist ) {
    const LintCombine::stringVector cmdLine = {
            "--sub-linter=MockLinterWrapper", "defaultLinter", "NotExistentFile",
            "--sub-linter=MockLinterWrapper", "defaultLinter", "NotExistentFile" };
    LintCombine::LinterCombine combine(
        cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    const auto callTotals = combine.updateYaml();
    BOOST_CHECK( callTotals.successNum == 0 );
    BOOST_CHECK( callTotals.failNum == 2 );

    BOOST_REQUIRE( combine.diagnostics().size() == 3 );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "LinterBase" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "YAML file path \"NotExistentFile\" doesn't exist" );
    const auto diagnostic_1 = combine.diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.origin == "LinterBase" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text == "YAML file path \"NotExistentFile\" doesn't exist" );
    const auto diagnostic_2 = combine.diagnostics()[2];
    BOOST_CHECK( diagnostic_2.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_2.origin == "Combine" );
    BOOST_CHECK( diagnostic_2.firstPos == 1 );
    BOOST_CHECK( diagnostic_2.lastPos == 0 );
    BOOST_CHECK( diagnostic_2.text == "Updating 2 YAML files failed" );
}

BOOST_AUTO_TEST_CASE( YamlPathExists ) {
    const LintCombine::stringVector cmdLine = {
            "--sub-linter=MockLinterWrapper", "defaultLinter",
            CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml" };
    LintCombine::LinterCombine combine( cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    auto callTotals = combine.updateYaml();
    BOOST_CHECK( callTotals.successNum == 1 );
    BOOST_CHECK( callTotals.failNum == 0 );
    std::ifstream yamlFile_1( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml" );
    std::ifstream yamlFile_1_save( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1_save.yaml" );
    std::istream_iterator < char > fileIter_1( yamlFile_1 ), end_1;
    std::istream_iterator < char > fileIter_1_save( yamlFile_1_save ), end_1_save;
    BOOST_CHECK_EQUAL_COLLECTIONS( fileIter_1, end_1, fileIter_1_save, end_1_save );
}

BOOST_AUTO_TEST_CASE( TwoLintersHaveExistYamlPath ) {
    LintCombine::stringVector cmdLine = {
            "--sub-linter=MockLinterWrapper", "defaultLinter",
            CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml",
            "--sub-linter=MockLinterWrapper", "defaultLinter",
            CURRENT_SOURCE_DIR"/yamlFiles/linterFile_2.yaml" };
    LintCombine::LinterCombine combine(
        cmdLine, LintCombine::MocksLinterFactory::getInstance() );
    auto callTotals = combine.updateYaml();
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

BOOST_FIXTURE_TEST_CASE( clangTidyUpdateYaml, recoverYamlFiles ) {
    LintCombine::stringVector cmdLine = {
            "--sub-linter=clang-tidy", "--export-fixes="
                                       CURRENT_SOURCE_DIR "/yamlFiles/linterFile_1.yaml" };
    LintCombine::LinterCombine combine( cmdLine );
    auto callTotals = combine.updateYaml();
    BOOST_CHECK( callTotals.successNum == 1 );
    BOOST_CHECK( callTotals.failNum == 0 );

    std::ifstream yamlFile_1( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1.yaml" );
    std::ifstream yamlFile_1_save( CURRENT_SOURCE_DIR"/yamlFiles/linterFile_1_result.yaml" );
    std::istream_iterator < char > fileIter_1( yamlFile_1 ), end_1;
    std::istream_iterator < char > fileIter_1_save( yamlFile_1_save ), end_1_save;
    BOOST_CHECK_EQUAL_COLLECTIONS( fileIter_1, end_1, fileIter_1_save, end_1_save );
}

BOOST_FIXTURE_TEST_CASE( clazyUpdateYaml, recoverYamlFiles ) {
    LintCombine::stringVector cmdLine = {
            "--sub-linter=clazy", "--export-fixes="
                                  CURRENT_SOURCE_DIR "/yamlFiles/linterFile_2.yaml" };
    LintCombine::LinterCombine combine( cmdLine );
    auto callTotals = combine.updateYaml();
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

BOOST_AUTO_TEST_CASE( OneLintersYamlPathNotExist ) {
    const std::string generalsYamlPath =
        CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml";
    const std::string lintersYamlPath = "NotExistentFile_1";
    const LintCombine::stringVector cmdLine = {
        "--result-yaml=" + generalsYamlPath,
        "--sub-linter=clang-tidy",
        "--export-fixes=" + lintersYamlPath };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.getYamlPath().empty() );
    BOOST_REQUIRE( !std::filesystem::exists( lintersYamlPath ) );
    BOOST_REQUIRE( !std::filesystem::exists( generalsYamlPath ) );
    BOOST_CHECK( combine.diagnostics().size() == 2 );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text ==
                 "Linter's YAML file path \"NotExistentFile_1\" doesn't exist" );
    const auto diagnostic_1 = combine.diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.origin == "Combine" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text == "General YAML file isn't created" );
}

BOOST_AUTO_TEST_CASE( OneLintersYamlPathExist ) {
    const std::string generalsYamlPath =
        CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml";
    const std::string lintersYamlPath =
        CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml";
    const LintCombine::stringVector cmdLine = {
        "--result-yaml=" + generalsYamlPath,
        "--sub-linter=clang-tidy",
        "--export-fixes=" + lintersYamlPath };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.getYamlPath() == generalsYamlPath );
    BOOST_REQUIRE( std::filesystem::exists( generalsYamlPath ) );
    BOOST_REQUIRE( std::filesystem::exists( lintersYamlPath ) );
    std::ifstream combinedResult_save( lintersYamlPath );
    std::ifstream combinedResult( generalsYamlPath );
    std::istream_iterator < char >
        combinedResult_saveIt( combinedResult_save ), endCR_save;
    std::istream_iterator < char >
        combinedResultIt( combinedResult ), endCR;
    BOOST_CHECK_EQUAL_COLLECTIONS( combinedResult_saveIt, endCR_save, combinedResultIt, endCR );
    combinedResult.close();
    std::filesystem::remove( generalsYamlPath );
    BOOST_CHECK( combine.diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( FirstLintersYamlPathExistSecondLintersYamlPathNotExist ) {
    const std::string generalsYamlPath =
        CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml";
    const std::string lintersYamlPath_1 =
        CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml";
    const std::string lintersYamlPath_2 = "NotExistentFile_2";
    const LintCombine::stringVector cmdLine = {
        "--result-yaml=" + generalsYamlPath,
        "--sub-linter=clang-tidy",
        "--export-fixes=" + lintersYamlPath_1,
        "--sub-linter=clang-tidy",
        "--export-fixes=" + lintersYamlPath_2 };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.getYamlPath() == generalsYamlPath );
    BOOST_REQUIRE( std::filesystem::exists( lintersYamlPath_1 ) );
    BOOST_REQUIRE( !std::filesystem::exists( lintersYamlPath_2 ) );
    BOOST_REQUIRE( std::filesystem::exists( generalsYamlPath ) );
    std::filesystem::remove( generalsYamlPath );
    BOOST_CHECK( combine.diagnostics().size() == 1 );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text ==
                 "Linter's YAML file path \"NotExistentFile_2\" doesn't exist" );
}

BOOST_AUTO_TEST_CASE( TwoLintersYamlPathNotExist ) {
    const std::string generalsYamlPath =
        CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml";
    const std::string lintersYamlPath_1 = "NotExistentFile_1";
    const std::string lintersYamlPath_2 = "NotExistentFile_2";
    const LintCombine::stringVector cmdLine = {
        "--result-yaml=" + generalsYamlPath,
        "--sub-linter=clang-tidy",
        "--export-fixes=" + lintersYamlPath_1,
        "--sub-linter=clang-tidy",
        "--export-fixes=" + lintersYamlPath_2 };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.getYamlPath().empty() );
    BOOST_REQUIRE( !std::filesystem::exists( lintersYamlPath_1 ) );
    BOOST_REQUIRE( !std::filesystem::exists( lintersYamlPath_2 ) );
    BOOST_REQUIRE( !std::filesystem::exists( generalsYamlPath ) );
    BOOST_CHECK( combine.diagnostics().size() == 3 );
    const auto diagnostic_0 = combine.diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_0.origin == "Combine" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text ==
                 "Linter's YAML file path \"NotExistentFile_1\" doesn't exist" );
    const auto diagnostic_1 = combine.diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_1.origin == "Combine" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text ==
                 "Linter's YAML file path \"NotExistentFile_2\" doesn't exist" );
    const auto diagnostic_2 = combine.diagnostics()[2];
    BOOST_CHECK( diagnostic_2.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_2.origin == "Combine" );
    BOOST_CHECK( diagnostic_2.firstPos == 1 );
    BOOST_CHECK( diagnostic_2.lastPos == 0 );
    BOOST_CHECK( diagnostic_2.text == "General YAML file isn't created" );
}

BOOST_AUTO_TEST_CASE( TwoLintersYamlPathExist ) {
    const std::string generalsYamlPath =
        CURRENT_SOURCE_DIR "yamlFiles/combinedResult.yaml";
    const std::string lintersYamlPath_1 =
        CURRENT_SOURCE_DIR "yamlFiles/linterFile_1.yaml";
    const std::string lintersYamlPath_2 =
        CURRENT_SOURCE_DIR "yamlFiles/linterFile_2.yaml";
    const LintCombine::stringVector cmdLine = {
        "--result-yaml=" + generalsYamlPath,
        "--sub-linter=clang-tidy",
        "--export-fixes=" + lintersYamlPath_1,
        "--sub-linter=clang-tidy",
        "--export-fixes=" + lintersYamlPath_2 };
    LintCombine::LinterCombine combine( cmdLine );
    BOOST_CHECK( combine.getYamlPath() == generalsYamlPath );
    BOOST_REQUIRE( std::filesystem::exists( generalsYamlPath ) );
    std::ifstream combinedResult_save(
        CURRENT_SOURCE_DIR"yamlFiles/combinedResult_save.yaml" );
    std::ifstream combinedResult( generalsYamlPath );
    std::istream_iterator < char >
        combinedResult_saveIt( combinedResult_save ), endCR_save;
    std::istream_iterator < char >
        combinedResultIt( combinedResult ), endCR;
    BOOST_CHECK_EQUAL_COLLECTIONS( combinedResult_saveIt, endCR_save, combinedResultIt, endCR );
    combinedResult.close();
    std::filesystem::remove( generalsYamlPath );
    BOOST_CHECK( combine.diagnostics().empty() );
}

BOOST_AUTO_TEST_SUITE_END()

#ifdef _WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

BOOST_AUTO_TEST_SUITE( TestPrepareCommandLine )

BOOST_AUTO_TEST_CASE( EmptyCommandLine ) {
    LintCombine::stringVector cmdLine = {};
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine = ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "FactoryPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Command Line is empty" );
}

BOOST_AUTO_TEST_CASE( FactoryDeleteIdeProfile_ValueAfterEqualSign ) {
    LintCombine::stringVector cmdLine = {
        "--param=value", "--ide-profile=resharper", "--param=value" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    const LintCombine::stringVector result = { "--param=value" , "--param=value" };
    compareContainers( cmdLine, result );
}

BOOST_AUTO_TEST_CASE( FactoryDeleteIdeProfile_ValueAfterSpace ) {
    LintCombine::stringVector cmdLine = {
        "--param=value", "--ide-profile", "clion", "--param=value" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( !ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    const LintCombine::stringVector result = { "--param=value", "--param=value" };
    compareContainers( cmdLine, result );
}

BOOST_AUTO_TEST_CASE( VerbatimLintersDontExist ) {
    LintCombine::stringVector cmdLine = {
        "--param=value", "-p=val", "--param", "val" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine = ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 2 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "VerbatimPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Options were passed verbatim" );
    const auto diagnostic_1 = prepareCmdLine->diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.origin == "VerbatimPreparer" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text ==
                 "No linters specified. Supported linters are: clang-tidy, clazy." );
}

BOOST_AUTO_TEST_CASE( VerbatimOneLinterWithIncorrectName ) {
    LintCombine::stringVector cmdLine = {
        "--sub-linter=Incorrect", "--param=value", "-p=val", "--param", "val" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine = ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 2 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "VerbatimPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Options were passed verbatim" );
    const auto diagnostic_1 = prepareCmdLine->diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.origin == "VerbatimPreparer" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text == "Unknown linter name: \"Incorrect\"" );
}

BOOST_AUTO_TEST_CASE( VerbatimTwoLintersWithIncorrectNames ) {
    LintCombine::stringVector cmdLine = {
        "--sub-linter=Incorrect_1", "--sub-linter=Incorrect_2",
        "--param=value", "-p=val", "--param", "val" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine = ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 3 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "VerbatimPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Options were passed verbatim" );
    const auto diagnostic_1 = prepareCmdLine->diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.origin == "VerbatimPreparer" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text == "Unknown linter name: \"Incorrect_1\"" );
    const auto diagnostic_2 = prepareCmdLine->diagnostics()[2];
    BOOST_CHECK( diagnostic_2.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_2.origin == "VerbatimPreparer" );
    BOOST_CHECK( diagnostic_2.firstPos == 1 );
    BOOST_CHECK( diagnostic_2.lastPos == 0 );
    BOOST_CHECK( diagnostic_2.text == "Unknown linter name: \"Incorrect_2\"" );
}

BOOST_AUTO_TEST_CASE( VerbatimOneLinterWithCorrectName ) {
    LintCombine::stringVector cmdLine = {
        "--result-yaml=file.yaml", "--sub-linter=clazy",
        "--param=value", "-p=val", "--param", "val" };
    const LintCombine::stringVector result = {
        "--result-yaml=file.yaml", "--sub-linter=clazy",
        "--param=value", "-p=val", "--param", "val" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine = ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "VerbatimPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Options were passed verbatim" );
}

BOOST_AUTO_TEST_CASE( VerbatimTwoLintersWithCorrectNames ) {
    LintCombine::stringVector cmdLine = {
        "--result-yaml=file.yaml", "--sub-linter=clazy",
        "--sub-linter=clang-tidy", "--param=value",
        "-p=val", "--param", "val" };
    const LintCombine::stringVector result = {
        "--result-yaml=file.yaml", "--sub-linter=clazy",
        "--sub-linter=clang-tidy", "--param=value",
        "-p=val", "--param", "val" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine = ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "VerbatimPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Options were passed verbatim" );
}

BOOST_AUTO_TEST_CASE( VerbatimResultYamlPathNotExists ) {
    LintCombine::stringVector cmdLine = {
        "--sub-linter=clazy", "--param=value", "-p=val", "--param", "val" };
    const LintCombine::stringVector result = {
        "--result-yaml=" CURRENT_BINARY_DIR "LintersDiagnostics.yaml",
        "--sub-linter=clazy", "--param=value",
        "-p=val", "--param", "val" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine = ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "VerbatimPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Options were passed verbatim" );
}

BOOST_AUTO_TEST_CASE( VerbatimInvalidResultYamlPath ) {
    LintCombine::stringVector cmdLine = {
        "--result-yaml=\\\\", "--sub-linter=clazy",
        "--param=value", "-p=val", "--param", "val" };
    const LintCombine::stringVector result = {
        "--result-yaml=" CURRENT_BINARY_DIR "LintersDiagnostics.yaml",
        "--sub-linter=clazy", "--param=value",
        "-p=val", "--param", "val" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine = ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 3 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "VerbatimPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Options were passed verbatim" );
    const auto diagnostic_1 = prepareCmdLine->diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_1.origin == "VerbatimPreparer" );
    BOOST_CHECK( diagnostic_1.firstPos == 1 );
    BOOST_CHECK( diagnostic_1.lastPos == 0 );
    BOOST_CHECK( diagnostic_1.text == "Incorrect general yaml filename: \"\\\\\"" );
    const auto diagnostic_2 = prepareCmdLine->diagnostics()[2];
    BOOST_CHECK( diagnostic_2.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_2.origin == "VerbatimPreparer" );
    BOOST_CHECK( diagnostic_2.firstPos == 1 );
    BOOST_CHECK( diagnostic_2.lastPos == 0 );
    BOOST_CHECK( diagnostic_2.text ==
                 "path to result-yaml changed to "
                 CURRENT_BINARY_DIR "LintersDiagnostics.yaml" );
}

BOOST_AUTO_TEST_CASE( UnsupportedIDE ) {
    LintCombine::stringVector cmdLine = { "--ide-profile=shasharper" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine = ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( !ideTraitsFactory.getIdeBehaviorInstance() );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "FactoryPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "\"shasharper\" is not a supported IDE profile" );
}

BOOST_AUTO_TEST_CASE( SpecifiedTwice ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase", "-p=pathToCompilationDataBase" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine = ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "option '--p' cannot be specified more than once" );
}

BOOST_AUTO_TEST_CASE( PathToYamlFileIsEmpty ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Path to yaml-file is empty." );
}

BOOST_AUTO_TEST_CASE( PathToCompilationDataBaseIsEmpty ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-export-fixes=pathToResultYaml" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "Path to compilation database is empty." );
}

static void minimalRequiredOptionsExistHelper( const std::string & ideName,
                                               const bool getDoesAddLinkValue,
                                               const bool linterExitCodeTolerant ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=" + ideName,
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml" };
    const LintCombine::stringVector result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClangTidy.yaml",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClazy.yaml" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesAddLink() == getDoesAddLinkValue );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesLinterExitCodeTolerant() == linterExitCodeTolerant );
    prepareCmdLine->transformCmdLine( cmdLine );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters are used" );
}

BOOST_AUTO_TEST_CASE( ReSharper_MinimalRequiredOptionsExist ) {
    minimalRequiredOptionsExistHelper( "ReSharper", true, false );
}

BOOST_AUTO_TEST_CASE( CLion_MinimalRequiredOptionsExist ) {
    if constexpr( BOOST_OS_WINDOWS ) {
        minimalRequiredOptionsExistHelper( "CLion", false, false );
    }
    if constexpr( BOOST_OS_LINUX ) {
        minimalRequiredOptionsExistHelper( "CLion", false, true );
    }
}

static void optionForClangTidyPassedHelper( const std::string & ideName,
                                            const bool getDoesAddLinkValue,
                                            const bool linterExitCodeTolerant ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--param_1", "@param_2" };

    const LintCombine::stringVector result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClangTidy.yaml",
        "--param_1", "@param_2",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClazy.yaml" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesAddLink() == getDoesAddLinkValue );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesLinterExitCodeTolerant() == linterExitCodeTolerant );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters are used" );
}

BOOST_AUTO_TEST_CASE( ReSharper_OptionForClangTidyPassed ) {
    optionForClangTidyPassedHelper( "ReSharper", true, false );
}

BOOST_AUTO_TEST_CASE( CLion_OptionForClangTidyPassed ) {
    if constexpr( BOOST_OS_WINDOWS ) {
        optionForClangTidyPassedHelper( "CLion", false, false );
    }
    if constexpr( BOOST_OS_LINUX ) {
        optionForClangTidyPassedHelper( "CLion", false, true );
    }
}

static void filesToAnalysisPassedHelper( const std::string & ideName,
                                         const bool getDoesAddLinkValue,
                                         const bool linterExitCodeTolerant ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "file_1.cpp", "file_2.cpp" };
    const LintCombine::stringVector result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClangTidy.yaml",
        "file_1.cpp", "file_2.cpp",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClazy.yaml",
        "file_1.cpp", "file_2.cpp" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesAddLink() == getDoesAddLinkValue );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesLinterExitCodeTolerant() == linterExitCodeTolerant );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters are used" );
}

BOOST_AUTO_TEST_CASE( ReSharper_FilesToAnalysisPassed ) {
    filesToAnalysisPassedHelper( "ReSharper", true, false );
}

BOOST_AUTO_TEST_CASE( CLion_FilesToAnalysisPassed ) {
    if constexpr( BOOST_OS_WINDOWS ) {
        filesToAnalysisPassedHelper( "CLion", false, false );
    }
    if constexpr( BOOST_OS_LINUX ) {
        filesToAnalysisPassedHelper( "CLion", false, true );
    }
}

BOOST_AUTO_TEST_CASE( ReSharper_HeaderFilterPassed ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--header-filter=file.cpp" };

    const LintCombine::stringVector result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClangTidy.yaml",
        "--header-filter=file.cpp",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClazy.yaml",
        "--header-filter=file.cpp" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters are used" );
}

BOOST_AUTO_TEST_CASE( ClazyChecksEmptyAfterEqualSign ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--clazy-checks=" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "FactoryPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text ==
                 "the argument for option '--clazy-checks'"
                 " should follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( ClangExtraArgsEmptyAfterEqualSign ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--clang-extra-args=" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "FactoryPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the argument for option '--clang-extra-args'"
                 " should follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( AllParamsEmptyAfterEqualSign ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--clazy-checks=", "--clang-extra-args=" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "FactoryPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text ==
                 "the argument for option '--clazy-checks'"
                 " should follow immediately after the equal sign" );
}

static void clazyChecksEmptyAfterSpaceHelper( const std::string & ideName,
                                              const bool getDoesAddLinkValue,
                                              const bool linterExitCodeTolerant ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--clazy-checks" };
    const LintCombine::stringVector result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClazy.yaml" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesAddLink() == getDoesAddLinkValue );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesLinterExitCodeTolerant() == linterExitCodeTolerant );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 2 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters are used" );
    const auto diagnostic_1 = prepareCmdLine->diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_1.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_1.firstPos == 63 );
    BOOST_CHECK( diagnostic_1.lastPos == 75 );
    BOOST_CHECK( diagnostic_1.text == "Parameter \"clazy-checks\" was set but "
                 "the parameter's value was not set. "
                 "The parameter will be ignored." );
}

BOOST_AUTO_TEST_CASE( ReSharper_ClazyChecksEmptyAfterSpace ) {
    clazyChecksEmptyAfterSpaceHelper( "ReSharper", true, false );
}

BOOST_AUTO_TEST_CASE( CLion_ClazyChecksEmptyAfterSpace ) {
    if constexpr( BOOST_OS_WINDOWS ) {
        clazyChecksEmptyAfterSpaceHelper( "CLion", false, false );
    }
    if constexpr( BOOST_OS_LINUX ) {
        clazyChecksEmptyAfterSpaceHelper( "CLion", false, true );
    }
}

static void clangExtraArgsEmptyAfterSpaceHelper( const std::string & ideName,
                                                 const bool getDoesAddLinkValue,
                                                 const bool linterExitCodeTolerant ) {
    LintCombine::stringVector cmdLine = {
            "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
            "--export-fixes=pathToResultYaml", "--clang-extra-args" };
    const LintCombine::stringVector result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClangTidy.yaml",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClazy.yaml" };
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesAddLink() == getDoesAddLinkValue );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesLinterExitCodeTolerant() == linterExitCodeTolerant );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 2 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters are used" );
    const auto diagnostic_1 = prepareCmdLine->diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_1.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_1.firstPos == 63 );
    BOOST_CHECK( diagnostic_1.lastPos == 79 );
    BOOST_CHECK( diagnostic_1.text ==
                 "Parameter \"clang-extra-args\" was set but "
                 "the parameter's value was not set. "
                 "The parameter will be ignored." );
}

BOOST_AUTO_TEST_CASE( ReSharper_ClangExtraArgsEmptyAfterSpace ) {
    clangExtraArgsEmptyAfterSpaceHelper( "ReSharper", true, false );
}

BOOST_AUTO_TEST_CASE( CLion_ClangExtraArgsEmptyAfterSpace ) {
    if constexpr( BOOST_OS_WINDOWS ) {
        clangExtraArgsEmptyAfterSpaceHelper( "CLion", false, false );
    }
    if constexpr( BOOST_OS_LINUX ) {
        clangExtraArgsEmptyAfterSpaceHelper( "CLion", false, true );
    }
}

static void allParamsEmptyAfterSpaceHelper( const std::string & ideName,
                                            const bool getDoesAddLinkValue,
                                            const bool linterExitCodeTolerant ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--clazy-checks", "--clang-extra-args" };
    const LintCombine::stringVector result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClangTidy.yaml",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClazy.yaml" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesAddLink() == getDoesAddLinkValue );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesLinterExitCodeTolerant() == linterExitCodeTolerant );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 3 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters are used" );
    const auto diagnostic_1 = prepareCmdLine->diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_1.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_1.firstPos == 63 );
    BOOST_CHECK( diagnostic_1.lastPos == 75 );
    BOOST_CHECK( diagnostic_1.text == "Parameter \"clazy-checks\" was set but "
                 "the parameter's value was not set. "
                 "The parameter will be ignored." );
    const auto diagnostic_2 = prepareCmdLine->diagnostics()[2];
    BOOST_CHECK( diagnostic_2.level == LintCombine::Level::Warning );
    BOOST_CHECK( diagnostic_2.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_2.firstPos == 78 );
    BOOST_CHECK( diagnostic_2.lastPos == 94 );
    BOOST_CHECK( diagnostic_2.text == "Parameter \"clang-extra-args\" was set but "
                 "the parameter's value was not set. "
                 "The parameter will be ignored." );
}

BOOST_AUTO_TEST_CASE( ReSharper_AllParamsEmptyAfterSpace ) {
    allParamsEmptyAfterSpaceHelper( "ReSharper", true, false );
}

BOOST_AUTO_TEST_CASE( CLion_AllParamsEmptyAfterSpace ) {
    if constexpr( BOOST_OS_WINDOWS ) {
        allParamsEmptyAfterSpaceHelper( "CLion", false, false );
    }
    if constexpr( BOOST_OS_LINUX ) {
        allParamsEmptyAfterSpaceHelper( "CLion", false, true );
    }
}

static void clazyChecksExistHelper( const std::string & ideName,
                                    const bool getDoesAddLinkValue,
                                    const bool linterExitCodeTolerant ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--clazy-checks", "level0,level1" };
    const LintCombine::stringVector result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClangTidy.yaml",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClazy.yaml",
        "--checks=level0,level1" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesAddLink() == getDoesAddLinkValue );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesLinterExitCodeTolerant() == linterExitCodeTolerant );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters are used" );
}

BOOST_AUTO_TEST_CASE( ReSharper_ClazyChecksExist ) {
    clazyChecksExistHelper( "ReSharper", true, false );
}

BOOST_AUTO_TEST_CASE( CLion_ClazyChecksExist ) {
    if constexpr( BOOST_OS_WINDOWS ) {
        clazyChecksExistHelper( "CLion", false, false );
    }
    if constexpr( BOOST_OS_LINUX ) {
        clazyChecksExistHelper( "CLion", false, true );
    }
}

static void clangExtraArgsExistHelper( const std::string & ideName,
                                       const bool getDoesAddLinkValue,
                                       const bool linterExitCodeTolerant ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--clang-extra-args=arg_1 arg_2 " };

    const LintCombine::stringVector result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClangTidy.yaml",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClazy.yaml",
        "--extra-arg=arg_1", "--extra-arg=arg_2" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesAddLink() == getDoesAddLinkValue );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesLinterExitCodeTolerant() == linterExitCodeTolerant );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters are used" );
}

BOOST_AUTO_TEST_CASE( ReSharper_ClangExtraArgsExist ) {
    clangExtraArgsExistHelper( "ReSharper", true, false );
}

BOOST_AUTO_TEST_CASE( CLion_ClangExtraArgsExist ) {
    if constexpr( BOOST_OS_WINDOWS ) {
        clangExtraArgsExistHelper( "CLion", false, false );
    }
    if constexpr( BOOST_OS_LINUX ) {
        clangExtraArgsExistHelper( "CLion", false, true );
    }
}

static void allParamsExistAfterEqualSignHelper( const std::string & ideName,
                                                const bool getDoesAddLinkValue,
                                                const bool linterExitCodeTolerant ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--clazy-checks=level0,level1",
        "--clang-extra-args=arg_1 arg_2" };

    const LintCombine::stringVector result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClangTidy.yaml",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClazy.yaml",
        "--checks=level0,level1",
        "--extra-arg=arg_1", "--extra-arg=arg_2" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesAddLink() == getDoesAddLinkValue );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesLinterExitCodeTolerant() == linterExitCodeTolerant );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters are used" );
}

BOOST_AUTO_TEST_CASE( ReSharper_AllParamsExistAfterEqualSign ) {
    allParamsExistAfterEqualSignHelper( "ReSharper", true, false );
}

BOOST_AUTO_TEST_CASE( CLion_AllParamsExistAfterEqualSign ) {
    if constexpr( BOOST_OS_WINDOWS ) {
        allParamsExistAfterEqualSignHelper( "CLion", false, false );
    }
    if constexpr( BOOST_OS_LINUX ) {
        allParamsExistAfterEqualSignHelper( "CLion", false, true );
    }
}

BOOST_AUTO_TEST_CASE( OneSublinterValueEmptyAfterEqualSign ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "FactoryPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text ==
                 "the argument for option '--sub-linter' should"
                 " follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( FirstSubLinterIncorrectSecondValueEmptyAfterEqualSign ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=IncorrectName_1",
        "--sub-linter=" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "FactoryPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text ==
                 "the argument for option '--sub-linter' should"
                 " follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( FirstSubLinterValueEmptyAfterEqualSignSecondIncorrect ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=",
        "--sub-linter=IncorrectName_1" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "FactoryPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text ==
                 "the argument for option '--sub-linter' should"
                 " follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( TwoSublinterValuesEmptyAfterEqualSign ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=", "--sub-linter=" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() == true );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "FactoryPreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "the argument for option '--sub-linter' should"
                 " follow immediately after the equal sign" );
}

BOOST_AUTO_TEST_CASE( OneSublinterEmptyValue ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text ==
                 "the required argument for option '--sub-linter' is missing" );
}

BOOST_AUTO_TEST_CASE( FirstSubLinterIncorrectSecondHasEmptyValueAfterSpace ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml",  "--sub-linter",
        "IncorrectName_1", "--sub-linter" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 1 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text ==
                 "the required argument for option '--sub-linter' is missing" );
}

BOOST_AUTO_TEST_CASE( FirstSubLinterHasEmptyValueAfterSpaceSecondIncorrect ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter",
        "--sub-linter", "IncorrectName_1" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 2 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters are used" );
    const auto diagnostic_1 = prepareCmdLine->diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_1.firstPos == 74 );
    BOOST_CHECK( diagnostic_1.lastPos == 86 );
    BOOST_CHECK( diagnostic_1.text == "Unknown linter name \"--sub-linter\"" );
}

BOOST_AUTO_TEST_CASE( AllSublintersHaveEmptyValueAfterSpace ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml",
        "--sub-linter", "--sub-linter" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 2 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters are used" );
    const auto diagnostic_1 = prepareCmdLine->diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_1.firstPos == 74 );
    BOOST_CHECK( diagnostic_1.lastPos == 86 );
    BOOST_CHECK( diagnostic_1.text == "Unknown linter name \"--sub-linter\"" );
}

BOOST_AUTO_TEST_CASE( OneSublinterWithIncorrectName ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=IncorrectName_1" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 2 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters are used" );
    const auto diagnostic_1 = prepareCmdLine->diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_1.firstPos == 74 );
    BOOST_CHECK( diagnostic_1.lastPos == 89 );
    BOOST_CHECK( diagnostic_1.text == "Unknown linter name \"IncorrectName_1\"" );
}

BOOST_AUTO_TEST_CASE( TwoSublinterWithIncorrectName ) {
    StreamCapture stderrCapture( std::cerr );
    LintCombine::stringVector cmdLine = {
        "--ide-profile=ReSharper", "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=IncorrectName_1",
        "--sub-linter=IncorrectName_2" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( prepareCmdLine->transformCmdLine( cmdLine ).empty() );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()->getDoesAddLink() );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().size() == 3 );
    const auto diagnostic_0 = prepareCmdLine->diagnostics()[0];
    BOOST_CHECK( diagnostic_0.level == LintCombine::Level::Info );
    BOOST_CHECK( diagnostic_0.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_0.firstPos == 1 );
    BOOST_CHECK( diagnostic_0.lastPos == 0 );
    BOOST_CHECK( diagnostic_0.text == "All linters are used" );
    const auto diagnostic_1 = prepareCmdLine->diagnostics()[1];
    BOOST_CHECK( diagnostic_1.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_1.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_1.firstPos == 74 );
    BOOST_CHECK( diagnostic_1.lastPos == 89 );
    BOOST_CHECK( diagnostic_1.text == "Unknown linter name \"IncorrectName_1\"" );
    const auto diagnostic_2 = prepareCmdLine->diagnostics()[2];
    BOOST_CHECK( diagnostic_2.level == LintCombine::Level::Error );
    BOOST_CHECK( diagnostic_2.origin == "BasePreparer" );
    BOOST_CHECK( diagnostic_2.firstPos == 103 );
    BOOST_CHECK( diagnostic_2.lastPos == 118 );
    BOOST_CHECK( diagnostic_2.text == "Unknown linter name \"IncorrectName_2\"" );
}

static void sublinterIsClangTidyHelper( const std::string & ideName,
                                        const bool getDoesAddLinkValue,
                                        const bool linterExitCodeTolerant ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=clang-tidy" };
    const LintCombine::stringVector result = {
            "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
            "-p=pathToCompilationDataBase",
            "--export-fixes=pathToCompilationDataBase"
            PATH_SEP "diagnosticsClangTidy.yaml" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesAddLink() == getDoesAddLinkValue );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesLinterExitCodeTolerant() == linterExitCodeTolerant );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( ReSharper_SublinterIsClangTidy ) {
    sublinterIsClangTidyHelper( "ReSharper", true, false );
}

BOOST_AUTO_TEST_CASE( CLion_SublinterIsClangTidy ) {
    if constexpr( BOOST_OS_WINDOWS ) {
        sublinterIsClangTidyHelper( "CLion", false, false );
    }
    if constexpr( BOOST_OS_LINUX ) {
        sublinterIsClangTidyHelper( "CLion", false, true );
    }
}

static void sublinterIsClazyHelper( const std::string & ideName,
                                    const bool getDoesAddLinkValue,
                                    const bool linterExitCodeTolerant ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=clazy" };
    const LintCombine::stringVector result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClazy.yaml" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesAddLink() == getDoesAddLinkValue );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesLinterExitCodeTolerant() == linterExitCodeTolerant );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( ReSharper_SublinterIsClazy ) {
    sublinterIsClazyHelper( "ReSharper", true, false );
}

BOOST_AUTO_TEST_CASE( CLion_SublinterIsClazy ) {
    if constexpr( BOOST_OS_WINDOWS ) {
        sublinterIsClazyHelper( "CLion", false, false );
    }
    if constexpr( BOOST_OS_LINUX ) {
        sublinterIsClazyHelper( "CLion", false, true );
    }
}

static void sublintersAreClangTidyAndClazyAfterEqualSignHelper( const std::string & ideName,
                                                                const bool getDoesAddLinkValue,
                                                                const bool linterExitCodeTolerant ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "--sub-linter=clang-tidy",
        "--sub-linter=clazy" };

    const LintCombine::stringVector result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClangTidy.yaml",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClazy.yaml" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesAddLink() == getDoesAddLinkValue );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesLinterExitCodeTolerant() == linterExitCodeTolerant );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( ReSharper_SublintersAreClangTidyAndClazyAfterEqualSign ) {
    sublintersAreClangTidyAndClazyAfterEqualSignHelper( "ReSharper", true, false );
}

BOOST_AUTO_TEST_CASE( CLion_SublintersAreClangTidyAndClazyAfterEqualSign ) {
    if constexpr( BOOST_OS_WINDOWS ) {
        sublintersAreClangTidyAndClazyAfterEqualSignHelper( "CLion", false, false );
    }
    if constexpr( BOOST_OS_LINUX ) {
        sublintersAreClangTidyAndClazyAfterEqualSignHelper( "CLion", false, true );
    }
}

static void sublintersAreClangTidyAndClazyAfterSpaceHelper( const std::string & ideName,
                                                            const bool getDoesAddLinkValue,
                                                            const bool linterExitCodeTolerant ) {
    LintCombine::stringVector cmdLine = {
        "--ide-profile=" + ideName, "-p=pathToCompilationDataBase",
        "--export-fixes=pathToResultYaml", "-sub-linter", "clang-tidy",
        "-sub-linter", "clazy" };
    const LintCombine::stringVector result = {
        "--result-yaml=pathToResultYaml",
        "--sub-linter=clang-tidy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClangTidy.yaml",
        "--sub-linter=clazy",
        "-p=pathToCompilationDataBase",
        "--export-fixes=pathToCompilationDataBase"
        PATH_SEP "diagnosticsClazy.yaml" };

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesAddLink() == getDoesAddLinkValue );
    BOOST_CHECK( ideTraitsFactory.getIdeBehaviorInstance()
                 ->getDoesLinterExitCodeTolerant() == linterExitCodeTolerant );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
    BOOST_REQUIRE( prepareCmdLine->diagnostics().empty() );
}

BOOST_AUTO_TEST_CASE( ReSharper_SublintersAreClangTidyAndClazyAfterSpace ) {
    sublintersAreClangTidyAndClazyAfterSpaceHelper( "ReSharper", true, false );
}

BOOST_AUTO_TEST_CASE( CLion_SublintersAreClangTidyAndClazyAfterSpace ) {
    if constexpr( BOOST_OS_WINDOWS ) {
        sublintersAreClangTidyAndClazyAfterSpaceHelper( "CLion", false, false );
    }
    if constexpr( BOOST_OS_LINUX ) {
        sublintersAreClangTidyAndClazyAfterSpaceHelper( "CLion", false, true );
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestSpecifyTargetArch )

void checkTargetArch( const std::string & macrosDir,
                      const std::string & targetTriple = std::string() ) {
    if constexpr( !BOOST_OS_WINDOWS ) {
        return;
    }
    LintCombine::stringVector cmdLine = {
        "--ide-profile=CLion", "-p=" CURRENT_SOURCE_DIR "CLionTestsMacros/"
        + macrosDir, "--export-fixes=pathToResultYaml"
    };
    LintCombine::stringVector result;
    if( !targetTriple.empty() ) {
        const std::string extraArg =
            "--extra-arg-before=\"--target=" + targetTriple + "\"";
        result = {
           "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
           "-p=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir,
           "--export-fixes=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir +
           PATH_SEP "diagnosticsClangTidy.yaml", extraArg,
            "--sub-linter=clazy",
           "-p=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir,
           "--export-fixes=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir +
           PATH_SEP "diagnosticsClazy.yaml", extraArg
        };
    }
    else {
        result = {
           "--result-yaml=pathToResultYaml",
            "--sub-linter=clang-tidy",
           "-p=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir,
           "--export-fixes=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir +
           PATH_SEP "diagnosticsClangTidy.yaml",
            "--sub-linter=clazy",
           "-p=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir,
           "--export-fixes=" CURRENT_SOURCE_DIR "CLionTestsMacros/" + macrosDir +
           PATH_SEP "diagnosticsClazy.yaml"
        };
    }

    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareCmdLine =
        ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    compareContainers( prepareCmdLine->transformCmdLine( cmdLine ), result );
}

BOOST_AUTO_TEST_CASE( MacrosFileDoesNotExist ) {
    const std::string macrosDir = "emptyDir";
    const std::string targetTriple;
    checkTargetArch( macrosDir, targetTriple );
}

BOOST_AUTO_TEST_CASE( EmptyMacrosFile ) {
    const std::string macrosDir = "emptyFileInsideDir";
    const std::string targetTriple;
    checkTargetArch( macrosDir, targetTriple );
}

BOOST_AUTO_TEST_CASE( ArchMacrosDontSpecified ) {
    const std::string macrosDir = "archMacrosDoNotSpecified";
    const std::string targetTriple;
    checkTargetArch( macrosDir, targetTriple );
}

BOOST_AUTO_TEST_CASE( SeveralDifferentArchSpecified ) {
    const std::string macrosDir = "severalDifferentArchSpecified";
    const std::string targetTriple;
    checkTargetArch( macrosDir, targetTriple );
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
