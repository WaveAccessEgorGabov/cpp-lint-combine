#define BOOST_TEST_MODULE lintWrapperTesting

#include "../LinterWrapperUtils.h"
#include "../LinterWrapperBase.h"
#include "../ClazyWrapper.h"
#include "../ClangTidyWrapper.h"
#include "yaml-cpp/yaml.h"

#include <boost/test/included/unit_test.hpp>
#include <filesystem>

struct initAddDocToYamlFileTests {
    initAddDocToYamlFileTests() {
        std::filesystem::remove( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" );
    }
};

struct MockWrapper : LinterWrapperBase {
    MockWrapper( const std::string & linterName, const std::string & linterOptions, const std::string & yamlFilePath )
            : LinterWrapperBase( linterOptions, yamlFilePath ) {
        this->linterName = linterName;
    }

    void addDocLinkToYaml( const YAML::Node & yamlNode ) override {}
};

BOOST_AUTO_TEST_SUITE( TestParseCommandLine )

    BOOST_AUTO_TEST_CASE( emptyCommandLine ) {
        char * str[] = { nullptr };
        LinterWrapperBase * res = parseCommandLine( sizeof ( str ) / sizeof( char * ), str );
        BOOST_CHECK( !res );
    }

    BOOST_AUTO_TEST_CASE( linterNotExists ) {
        char * str[] = { nullptr, "-L", "mockLinter" };
        LinterWrapperBase * res = parseCommandLine( sizeof ( str ) / sizeof( char * ), str );
        BOOST_CHECK( !res );
    }

    BOOST_AUTO_TEST_CASE( linterClangYidy ) {
        char * str[] = { nullptr, "-L", "clang-tidy" };
        LinterWrapperBase * res = parseCommandLine( sizeof ( str ) / sizeof( char * ), str );
        BOOST_CHECK( res );
        BOOST_CHECK( res->getLinterName () == "clang-tidy" );
        BOOST_CHECK( res->getLinterOptions ().empty () );
        BOOST_CHECK( res->getYamlFilePath ().empty () );
    }

    BOOST_AUTO_TEST_CASE( linterClazy ) {
        char * str[] = { nullptr, "-L", "clazy-standalone" };
        LinterWrapperBase * res = parseCommandLine( sizeof ( str ) / sizeof( char * ), str );
        BOOST_CHECK( res );
        BOOST_CHECK( res->getLinterName () == "clazy-standalone" );
        BOOST_CHECK( res->getLinterOptions ().empty () );
        BOOST_CHECK( res->getYamlFilePath ().empty () );
    }

    BOOST_AUTO_TEST_CASE( LinterOptionsExists ) {
        char * str[] = { nullptr, "-L", "clazy-standalone", "param1", "param2", "param3" };
        LinterWrapperBase * res = parseCommandLine( sizeof ( str ) / sizeof( char * ), str );
        BOOST_CHECK( res );
        BOOST_CHECK( res->getLinterName () == "clazy-standalone" );
        BOOST_CHECK( res->getLinterOptions () == "param1 param2 param3 " );
        BOOST_CHECK( res->getYamlFilePath ().empty () );
    }

    BOOST_AUTO_TEST_CASE( yamlFileExist ) {
        char * str[] = { nullptr, "-L", "clazy-standalone", "--export-fixes", "file.yaml" };
        LinterWrapperBase * res = parseCommandLine( sizeof ( str ) / sizeof( char * ), str );
        BOOST_CHECK( res );
        BOOST_CHECK( res->getLinterName () == "clazy-standalone" );
        BOOST_CHECK( res->getLinterOptions ().empty () );
        BOOST_CHECK( res->getYamlFilePath () == "file.yaml" );
    }

    BOOST_AUTO_TEST_CASE( yamlFileAndlinterOptionsExist ) {
        char * str[] = { nullptr, "--linter", "clazy-standalone", "--export-fixes", "file.yaml", "param1", "param2", "param3" };
        LinterWrapperBase * res = parseCommandLine( sizeof ( str ) / sizeof( char * ), str );
        BOOST_CHECK( res );
        BOOST_CHECK( res->getLinterName () == "clazy-standalone" );
        BOOST_CHECK( res->getLinterOptions () == "param1 param2 param3 " );
        BOOST_CHECK( res->getYamlFilePath () == "file.yaml" );
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestCallLinter )

    BOOST_AUTO_TEST_CASE( emptyParameters ) {
        MockWrapper linterWrapper( "", "", "" );
        BOOST_CHECK( linterWrapper.callLinter () == 1 );
    }

    BOOST_AUTO_TEST_CASE( linterNotExists ) {
        MockWrapper linterWrapper( "nonexistLinter", "", "" );
        BOOST_CHECK( linterWrapper.callLinter () == 1 );
    }

    BOOST_AUTO_TEST_CASE( linterReturn0 ) {
#ifdef WIN32
        MockWrapper linterWrapper( CURRENT_BINARY_DIR"Debug/mockReturn0", "", "" );
#elif __linux__
        MockWrapper linterWrapper( CURRENT_BINARY_DIR"mockReturn0", "", "" );
#endif
        BOOST_CHECK( linterWrapper.callLinter() == 0 );
    }

    BOOST_AUTO_TEST_CASE( linterReturn1 ) {
#ifdef WIN32
        MockWrapper linterWrapper( CURRENT_BINARY_DIR"Debug/mockReturn1", "", "" );
#elif __linux__
        MockWrapper linterWrapper( CURRENT_BINARY_DIR"mockReturn1", "", "" );
#endif
        BOOST_CHECK( linterWrapper.callLinter () == 1 );
    }

    BOOST_AUTO_TEST_CASE( linterReturn2 ) {
#ifdef WIN32
        MockWrapper linterWrapper( CURRENT_BINARY_DIR"Debug/mockReturn2", "", "" );
#elif __linux__
        MockWrapper linterWrapper( CURRENT_BINARY_DIR"mockReturn2", "", "" );
#endif
        BOOST_CHECK( linterWrapper.callLinter () == 2 );
    }

    BOOST_AUTO_TEST_CASE( linterReturn0AndYamlExists ) {
#ifdef WIN32
        MockWrapper linterWrapper( CURRENT_BINARY_DIR"Debug/mockReturn0", "", "test.yaml" );
#elif __linux__
        MockWrapper linterWrapper( CURRENT_BINARY_DIR"mockReturn0", "", "test.yaml" );
#endif
        BOOST_CHECK( linterWrapper.callLinter () == 0 );
    }

    BOOST_AUTO_TEST_CASE( linterReturn0AndLinterOptionsExists ) {
#ifdef WIN32
        MockWrapper linterWrapper( CURRENT_BINARY_DIR"Debug/mockReturn0", "param1 param2", "" );
#elif __linux__
        MockWrapper linterWrapper( CURRENT_BINARY_DIR"mockReturn0", "param1 param2", "" );
#endif
        BOOST_CHECK( linterWrapper.callLinter () == 0 );
    }

    BOOST_AUTO_TEST_CASE( linterReturn0YamFileAndLinterOptionsExists ) {
#ifdef WIN32
        MockWrapper linterWrapper( CURRENT_BINARY_DIR"Debug/mockReturn0", "param1 param2", "test.yaml" );
#elif __linux__
        MockWrapper linterWrapper( CURRENT_BINARY_DIR"mockReturn0", "param1 param2", "test.yaml" );
#endif
        BOOST_CHECK( linterWrapper.callLinter () == 0 );
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TestAddDocLinkToYamlFile )

    BOOST_FIXTURE_TEST_CASE( linterAndYamlFileNotExist, initAddDocToYamlFileTests ) {
        MockWrapper mockWrapper( "SomeNotExistentProgram", "", "SomeNotExistentFile" );
        BOOST_CHECK( !mockWrapper.createUpdatedYaml () );
        for( const auto & contourFile : std::filesystem::directory_iterator( CURRENT_SOURCE_DIR ) ) {
            BOOST_CHECK( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" != contourFile.path() );
        }
    }

    BOOST_FIXTURE_TEST_CASE( linterNotExistsYamlEmpty, initAddDocToYamlFileTests ) {
        MockWrapper mockWrapper( "SomeNotExistentProgram", "", "" );
        BOOST_CHECK( !mockWrapper.createUpdatedYaml () );
        for( const auto & contourFile : std::filesystem::directory_iterator( CURRENT_SOURCE_DIR ) ) {
            BOOST_CHECK( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" != contourFile.path() );
        }
    }

    BOOST_FIXTURE_TEST_CASE( ClangTidyYamlEmpty, initAddDocToYamlFileTests ) {
        ClangTidyWrapper clangTidyWripper( "", "" );
        BOOST_CHECK( !clangTidyWripper.createUpdatedYaml() );
        for( const auto & contourFile : std::filesystem::directory_iterator( CURRENT_SOURCE_DIR ) ) {
            BOOST_CHECK( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" != contourFile.path() );
        }
    }

    BOOST_FIXTURE_TEST_CASE( ClazyStandaloneYamlEmpty, initAddDocToYamlFileTests ) {
        ClazyWrapper clazyWrapper( "", "" );
        BOOST_CHECK( !clazyWrapper.createUpdatedYaml () );
        for( const auto & contourFile : std::filesystem::directory_iterator( CURRENT_SOURCE_DIR ) ) {
            BOOST_CHECK( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" != contourFile.path() );
        }
    }

    BOOST_FIXTURE_TEST_CASE( mockProgrammYamlExists, initAddDocToYamlFileTests ) {
        MockWrapper mockWrapper( "SomeNotExistentProgram", "", CURRENT_SOURCE_DIR"yamlFiles/clangTidy.yaml" );
        BOOST_CHECK( mockWrapper.createUpdatedYaml () );
        bool isYamlFileExists = false;
        for( const auto & contourFile : std::filesystem::directory_iterator( CURRENT_SOURCE_DIR ) ) {
            if( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" == contourFile.path() ) {
                isYamlFileExists = true;
            }
        }
        BOOST_REQUIRE( isYamlFileExists );

        YAML::Node yamlNode;
        yamlNode = YAML::LoadFile( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" );
        for( auto it : yamlNode[ "Diagnostics" ] ) {
            std::ostringstream ossDocLink;
            ossDocLink << it[ "Documentation link" ];
            BOOST_CHECK( ossDocLink.str().empty () );
        }
    }

    BOOST_FIXTURE_TEST_CASE( ClangTidyYamlExists, initAddDocToYamlFileTests ) {
        ClangTidyWrapper clangTidyWripper( "", CURRENT_SOURCE_DIR"yamlFiles/clangTidy.yaml" );
        BOOST_CHECK( clangTidyWripper.createUpdatedYaml () );
        bool isYamlFileExists = false;
        for( const auto & contourFile : std::filesystem::directory_iterator( CURRENT_SOURCE_DIR ) ) {
            if( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" == contourFile.path() ) {
                isYamlFileExists = true;
            }
        }
        BOOST_CHECK( isYamlFileExists );

        YAML::Node yamlNode;
        yamlNode = YAML::LoadFile( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" );
        for( auto it : yamlNode[ "Diagnostics" ] ) {
            std::ostringstream ossDocLink;
            ossDocLink << it[ "Documentation link" ];
            std::ostringstream ossToCompare;
            ossToCompare << "https://clang.llvm.org/extra/clang-tidy/checks/" << it[ "DiagnosticName" ] << ".html";
            BOOST_CHECK( ossDocLink.str () == ossToCompare.str () );
        }
    }

    BOOST_FIXTURE_TEST_CASE( ClazyStandaloneYamlExists, initAddDocToYamlFileTests ) {
        ClazyWrapper clazyWrapper( "", CURRENT_SOURCE_DIR"yamlFiles/clangTidy.yaml" );
        BOOST_CHECK( clazyWrapper.createUpdatedYaml () );
        bool isYamlFileExists = false;
        for( const auto & contourFile : std::filesystem::directory_iterator( CURRENT_SOURCE_DIR ) ) {
            if( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" == contourFile.path() ) {
                isYamlFileExists = true;
            }
        }
        BOOST_CHECK( isYamlFileExists );

        YAML::Node yamlNode;
        yamlNode = YAML::LoadFile( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" );
        for( auto it : yamlNode[ "Diagnostics" ] ) {
            std::ostringstream ossDocLink;
            ossDocLink << it[ "Documentation link" ];
            std::ostringstream tempOss;
            tempOss << it[ "DiagnosticName" ];
            std::ostringstream ossToCompare;
            ossToCompare << "https://github.com/KDE/clazy/blob/master/docs/checks/README-";
            // substr() from 6 to size() for skipping "clazy-" in DiagnosticName
            ossToCompare << tempOss.str().substr( 6, tempOss.str().size() ) << ".md";
            BOOST_CHECK( ossDocLink.str() == ossToCompare.str () );
        }
    }

BOOST_AUTO_TEST_SUITE_END()
