#define BOOST_TEST_MODULE lintWrapperTesting

#include <boost/process.hpp>

#include "../LinterWrapperUtils.h"
#include "../ClazyWrapper.h"
#include "../ClangTidyWrapper.h"
#include "../LinterSwitch.h"
#include "yaml-cpp/yaml.h"

#include <boost/test/included/unit_test.hpp>
#include <memory>
#include <filesystem>

struct initAddDocToYamlFileTests {
    initAddDocToYamlFileTests() {
        std::filesystem::remove( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" );
    }

    ~initAddDocToYamlFileTests() {
        std::filesystem::remove( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" );
    }
};

struct MockWrapper : LinterWrapperBase {
    MockWrapper( const std::string & linterName, const std::string & linterOptions, const std::string & yamlFilePath )
            : LinterWrapperBase( linterOptions, yamlFilePath ) {
        this->linterName = linterName;
    }

    void addDocLinkToYaml( const YAML::Node & yamlNode ) const override {}
};

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

BOOST_AUTO_TEST_SUITE( TestParseCommandLine )

    BOOST_AUTO_TEST_CASE( emptyCommandLine ) {
        char * str[] = { nullptr };
        bool isNeedHelp;
        LinterWrapperItf * res = parseCommandLine( sizeof( str ) / sizeof( char * ), str, isNeedHelp );
        BOOST_CHECK( !res );
    }

    BOOST_AUTO_TEST_CASE( linterNotExists ) {
        char * str[] = { nullptr, "-L", "notExistenLinter" };
        bool isNeedHelp;
        LinterWrapperItf * res = parseCommandLine( sizeof( str ) / sizeof( char * ), str, isNeedHelp );
        BOOST_CHECK( !res );
    }

    BOOST_AUTO_TEST_CASE( linterClangTidy ) {
        char * str[] = { nullptr, "-L", "clang-tidy" };
        bool isNeedHelp;
        LinterWrapperBase * res = parseCommandLine( sizeof( str ) / sizeof( char * ), str, isNeedHelp );
        BOOST_CHECK( res );
#ifdef WIN32
        BOOST_CHECK( res->getLinterName() == "clang-tidy.exe" );
#elif __linux__
        BOOST_CHECK( res->getLinterName() == "clang-tidy" );
#endif
        BOOST_CHECK( res->getLinterOptions().empty() );
        BOOST_CHECK( res->getYamlFilePath().empty() );
    }

    BOOST_AUTO_TEST_CASE( linterClazy ) {
        char * str[] = { nullptr, "-L", "clazy-standalone" };
        bool isNeedHelp;
        LinterWrapperBase * res = parseCommandLine( sizeof( str ) / sizeof( char * ), str, isNeedHelp );
        BOOST_CHECK( res );
#ifdef WIN32
        BOOST_CHECK(res->getLinterName() == "clazy-standalone.exe" );
#elif __linux__
        BOOST_CHECK( res->getLinterName() == "clazy-standalone" );
#endif
        BOOST_CHECK( res->getLinterOptions().empty() );
        BOOST_CHECK( res->getYamlFilePath().empty() );
    }

    BOOST_AUTO_TEST_CASE( LinterOptionsExists ) {
        char * str[] = { nullptr, "-L", "clazy-standalone", "param1", "param2", "param3" };
        bool isNeedHelp;
        LinterWrapperBase * res = parseCommandLine( sizeof( str ) / sizeof( char * ), str, isNeedHelp );
        BOOST_CHECK( res );
#ifdef WIN32
        BOOST_CHECK(res->getLinterName() == "clazy-standalone.exe");
#elif __linux__
        BOOST_CHECK( res->getLinterName() == "clazy-standalone" );
#endif
        BOOST_CHECK( res->getLinterOptions() == "param1 param2 param3 " );
        BOOST_CHECK( res->getYamlFilePath().empty() );
    }

    BOOST_AUTO_TEST_CASE( yamlFileExist ) {
        char * str[] = { nullptr, "-L", "clazy-standalone", "--export-fixes", "file.yaml" };
        bool isNeedHelp;
        LinterWrapperBase * res = parseCommandLine( sizeof( str ) / sizeof( char * ), str, isNeedHelp );
        BOOST_CHECK( res );
#ifdef WIN32
        BOOST_CHECK(res->getLinterName() == "clazy-standalone.exe");
#elif __linux__
        BOOST_CHECK( res->getLinterName() == "clazy-standalone" );
#endif
        BOOST_CHECK( res->getLinterOptions().empty() );
        BOOST_CHECK( res->getYamlFilePath() == "file.yaml" );
    }

    BOOST_AUTO_TEST_CASE( yamlFileAndLinterOptionsExist ) {
        char * str[] = { nullptr, "--linter", "clazy-standalone", "--export-fixes", "file.yaml", "param1", "param2",
                         "param3" };
        bool isNeedHelp;
        LinterWrapperBase * res = parseCommandLine( sizeof( str ) / sizeof( char * ), str, isNeedHelp );
        BOOST_CHECK( res );
#ifdef WIN32
        BOOST_CHECK(res->getLinterName() == "clazy-standalone.exe");
#elif __linux__
        BOOST_CHECK( res->getLinterName() == "clazy-standalone" );
#endif
        BOOST_CHECK( res->getLinterOptions() == "param1 param2 param3 " );
        BOOST_CHECK( res->getYamlFilePath() == "file.yaml" );
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestCallLinter )

    BOOST_AUTO_TEST_CASE( mockProgrammTerminated ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
#ifdef WIN32
        const std::string scriptRunner = "sh.exe ";
#elif __linux__
        const std::string scriptRunner = boost::process::shell().string();
#endif
        auto linterWrapper = std::make_shared < MockWrapper >( scriptRunner
                + " " + CURRENT_SOURCE_DIR"mockPrograms/mockTerminated.sh", "", "" );
        LinterSwitch linter( linterWrapper );
        int returnCode = linter.callLinter( false );
        BOOST_CHECK( stdoutCapture.getBufferData() == "stdout\n" );
        BOOST_CHECK( stderrCapture.getBufferData() == "stderr\n" );
        BOOST_CHECK( returnCode == 9 );
    }

    BOOST_AUTO_TEST_CASE( linterReturn1 ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
#ifdef WIN32
        const std::string scriptRunner = "sh.exe ";
#elif __linux__
        const std::string scriptRunner = boost::process::shell().string();
#endif
        auto linterWrapper = std::make_shared < MockWrapper >( scriptRunner
                + " " CURRENT_SOURCE_DIR"mockPrograms/mockReturn1.sh", "", "" );
        LinterSwitch linter( linterWrapper );
        int returnCode = linter.callLinter( false );
        BOOST_CHECK( stdoutCapture.getBufferData() == "stdout\n" );
        BOOST_CHECK( stderrCapture.getBufferData() == "stderr\n" );
        BOOST_CHECK( returnCode == 1 );
    }

    BOOST_AUTO_TEST_CASE( EmptyParameters ) {
        auto linterWrapper = std::make_shared < MockWrapper >( "", "", "" );
        LinterSwitch linter( linterWrapper );
        int returnCode = linter.callLinter( false );
        BOOST_CHECK( returnCode == 1 );
    }

    BOOST_AUTO_TEST_CASE( NotExistenLinter ) {
        auto linterWrapper = std::make_shared < MockWrapper >( "notExistFile", "", "" );
        LinterSwitch linter( linterWrapper );
        int returnCode = linter.callLinter( false );
        BOOST_CHECK( returnCode == 1 );
    }

    BOOST_AUTO_TEST_CASE( linterReturn0AndWriteToStreams ) {
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
#ifdef WIN32
        const std::string scriptRunner = "sh.exe ";
#elif __linux__
        const std::string scriptRunner = boost::process::shell().string();
#endif
        auto linterWrapper = std::make_shared < MockWrapper >( scriptRunner
                + " " CURRENT_SOURCE_DIR"mockPrograms/mockReturn0AndWriteToStreams.sh", "", "" );
        LinterSwitch linter( linterWrapper );
        int returnCode = linter.callLinter( false );
        BOOST_CHECK( stdoutCapture.getBufferData() == "stdout\n" );
        BOOST_CHECK( stderrCapture.getBufferData() == "stderr\n" );
        BOOST_CHECK( returnCode == 0 );
    }

    BOOST_AUTO_TEST_CASE( linterReturn0AndWriteToFile ) {
        std::filesystem::remove( CURRENT_BINARY_DIR"test.yaml" );
#ifdef WIN32
        const std::string scriptRunner = "sh.exe ";
#elif __linux__
        const std::string scriptRunner = boost::process::shell().string();
#endif
        auto linterWrapper = std::make_shared < MockWrapper >(scriptRunner
                + " " CURRENT_SOURCE_DIR"mockPrograms/mockReturn0AndWriteToFile.sh .sh", "", "" );
        LinterSwitch linter( linterWrapper );
        int returnCode = linter.callLinter( false );
        BOOST_CHECK( returnCode == 0 );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"test.yaml" ) );
        std::string str;
        getline( std::fstream( CURRENT_BINARY_DIR"test.yaml" ), str );
        BOOST_CHECK( str == "this is yaml-file" );
        std::filesystem::remove( CURRENT_BINARY_DIR"test.yaml" );
    }

    BOOST_AUTO_TEST_CASE( linterReturn0AndWriteToFileAndToStream ) {
        std::filesystem::remove( CURRENT_BINARY_DIR"test.yaml" );
        StreamCapture stdoutCapture( std::cout );
        StreamCapture stderrCapture( std::cerr );
#ifdef WIN32
        const std::string scriptRunner = "sh.exe ";
#elif __linux__
        const std::string scriptRunner = boost::process::shell().string();
#endif
        auto linterWrapper = std::make_shared < MockWrapper >( scriptRunner
                + " " CURRENT_SOURCE_DIR"mockPrograms/mockReturn0AndWriteToStreamsAndToFile.sh", "", "" );
        LinterSwitch linter( linterWrapper );
        int returnCode = linter.callLinter( false );
        BOOST_CHECK( stdoutCapture.getBufferData() == "stdout\n" );
        BOOST_CHECK( stderrCapture.getBufferData() == "stderr\n" );
        BOOST_CHECK( returnCode == 0 );
        BOOST_REQUIRE( std::filesystem::exists( CURRENT_BINARY_DIR"test.yaml" ) );
        std::string str;
        getline( std::fstream( CURRENT_BINARY_DIR"test.yaml" ), str );
        BOOST_CHECK( str == "this is yaml-file" );
        std::filesystem::remove( CURRENT_BINARY_DIR"test.yaml" );
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestAddDocLinkToYamlFile )

    BOOST_FIXTURE_TEST_CASE( linterAndYamlFileNotExist, initAddDocToYamlFileTests ) {
        auto linterWrapper = std::make_shared < MockWrapper >( "notExistProgram", "", "notExistFile" );
        LinterSwitch linter( linterWrapper );
        BOOST_CHECK( !linter.createUpdatedYaml() );
        BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" ) );
    }

    BOOST_FIXTURE_TEST_CASE( linterNotExistsYamlEmpty, initAddDocToYamlFileTests ) {
        auto linterWrapper = std::make_shared < MockWrapper >( "notExistProgram", "", "" );
        LinterSwitch linter( linterWrapper );
        BOOST_CHECK( !linter.createUpdatedYaml() );
        BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" ) );
    }

    BOOST_FIXTURE_TEST_CASE( ClangTidyYamlEmpty, initAddDocToYamlFileTests ) {
        auto linterWrapper = std::make_shared < ClangTidyWrapper >( "", "" );
        LinterSwitch linter( linterWrapper );
        BOOST_CHECK( !linter.createUpdatedYaml() );
        BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" ) );
    }

    BOOST_FIXTURE_TEST_CASE( ClazyStandaloneYamlEmpty, initAddDocToYamlFileTests ) {
        auto linterWrapper = std::make_shared < ClazyWrapper >( "", "" );
        LinterSwitch linter( linterWrapper );
        BOOST_CHECK( !linter.createUpdatedYaml() );
        BOOST_CHECK( !std::filesystem::exists( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" ) );
    }

    BOOST_FIXTURE_TEST_CASE( mockProgrammYamlExists, initAddDocToYamlFileTests ) {
        auto linterWrapper
                = std::make_shared < MockWrapper >( "notExistProgram", "",
                        CURRENT_SOURCE_DIR"yamlFiles/clangTidy.yaml" );
        LinterSwitch linter( linterWrapper );
        BOOST_CHECK( linter.createUpdatedYaml() );
        BOOST_CHECK( std::filesystem::exists( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" ) );

        YAML::Node yamlNode;
        yamlNode = YAML::LoadFile( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" );
        for( auto it : yamlNode[ "Diagnostics" ] ) {
            std::ostringstream documentationLink;
            documentationLink << it[ "Documentation link" ];
            BOOST_CHECK( documentationLink.str().empty() );
        }
    }

    BOOST_FIXTURE_TEST_CASE( ClangTidyYamlExists, initAddDocToYamlFileTests ) {
        auto linterWrapper
                = std::make_shared < ClangTidyWrapper >( "", CURRENT_SOURCE_DIR"yamlFiles/clangTidy.yaml" );
        LinterSwitch linter( linterWrapper );
        BOOST_CHECK( linter.createUpdatedYaml() );
        BOOST_CHECK( std::filesystem::exists( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" ) );

        YAML::Node yamlNode;
        yamlNode = YAML::LoadFile( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" );
        for( auto it : yamlNode[ "Diagnostics" ] ) {
            std::ostringstream documentationLink;
            documentationLink << it[ "Documentation link" ];
            std::ostringstream ossToCompare;
            ossToCompare << "https://clang.llvm.org/extra/clang-tidy/checks/" << it[ "DiagnosticName" ] << ".html";
            BOOST_CHECK( documentationLink.str() == ossToCompare.str() );
        }
    }

    BOOST_FIXTURE_TEST_CASE( ClazyStandaloneYamlExists, initAddDocToYamlFileTests ) {
        auto linterWrapper
                = std::make_shared < ClazyWrapper >( "", CURRENT_SOURCE_DIR"yamlFiles/clazy.yaml" );
        LinterSwitch linter( linterWrapper );
        BOOST_CHECK( linter.createUpdatedYaml() );
        BOOST_CHECK( std::filesystem::exists( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" ) );

        YAML::Node yamlNode;
        yamlNode = YAML::LoadFile( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" );
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
