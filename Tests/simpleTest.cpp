#define BOOST_TEST_MODULE lintWrapperTesting

#include "../lintWrapperUtils.h"
#include "yaml-cpp/yaml.h"

#include <boost/test/included/unit_test.hpp>
#include <filesystem>

struct initParseAndCallLinterTests {
    std::string linterName;
    std::string yamlFilePath;
    std::string linterOptions;
};

struct initAddDocToYamlFileTests {
    initAddDocToYamlFileTests() {
        std::filesystem::remove(CURRENT_SOURCE_DIR"/../linterYamlWithDocLink.yaml");
    }
    std::string linterName;
    std::string yamlFilePath;
};

BOOST_AUTO_TEST_SUITE( TestParseCommandLine )
    BOOST_FIXTURE_TEST_CASE(emptyCommandLine, initParseAndCallLinterTests)
    {
        char* str[] = {0};
        parseCommandLine(sizeof(str)/sizeof(char*), str, linterName, yamlFilePath, linterOptions);
        BOOST_CHECK(linterName.empty());
        BOOST_CHECK(yamlFilePath.empty());
        BOOST_CHECK(linterOptions.empty());
    }

    BOOST_FIXTURE_TEST_CASE(linterExists, initParseAndCallLinterTests )
    {
        char* str[] = {0, "-L", "mockLinter"};
        parseCommandLine(sizeof(str)/sizeof(char*), str, linterName, yamlFilePath, linterOptions);
        BOOST_CHECK(linterName == "mockLinter");
        BOOST_CHECK(yamlFilePath.empty());
        BOOST_CHECK(linterOptions.empty());
    }

    BOOST_FIXTURE_TEST_CASE(LinterOptionsExists, initParseAndCallLinterTests )
    {
        char* str[] = {0, "param1", "param2", "param3"};
        parseCommandLine(sizeof(str)/sizeof(char*), str, linterName, yamlFilePath, linterOptions);
        BOOST_CHECK(linterName.empty());
        BOOST_CHECK(yamlFilePath.empty());
        BOOST_CHECK(linterOptions == "param1 param2 param3 ");
    }

    BOOST_FIXTURE_TEST_CASE(linterAndYamlFile_Exist, initParseAndCallLinterTests )
    {
        char* str[] = {0, "-L", "mockLinter", "--export-fixes", "file.yaml"};
        parseCommandLine(sizeof(str)/sizeof(char*), str, linterName, yamlFilePath, linterOptions);
        BOOST_CHECK(linterName == "mockLinter");
        BOOST_CHECK(yamlFilePath == "file.yaml");
        BOOST_CHECK(linterOptions.empty());
    }

    BOOST_FIXTURE_TEST_CASE(linterAndYamlFileAndlinterOptionsExist, initParseAndCallLinterTests )
    {
        char* str[] = {0, "--linter", "mockLinter", "--export-fixes", "file.yaml", "param1", "param2", "param3"};
        parseCommandLine(sizeof(str)/sizeof(char*), str, linterName, yamlFilePath, linterOptions);
        BOOST_CHECK(linterName == "mockLinter");
        BOOST_CHECK(yamlFilePath == "file.yaml");
        BOOST_CHECK(linterOptions == "param1 param2 param3 ");
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(TestCallLinter)
    BOOST_FIXTURE_TEST_CASE(emptyParameters, initParseAndCallLinterTests)
    {
        BOOST_CHECK(callLinter(linterName, yamlFilePath, linterOptions) == 1);
    }

    BOOST_FIXTURE_TEST_CASE(linterNotExists, initParseAndCallLinterTests)
    {
        linterName = "someNotExistentName";
        BOOST_CHECK(callLinter(linterName, yamlFilePath, linterOptions) == 1);
    }

    BOOST_FIXTURE_TEST_CASE(linterReturn0, initParseAndCallLinterTests)
    {
        linterName = CURRENT_BINARY_DIR"/mockReturn0";
        BOOST_CHECK(callLinter(linterName, yamlFilePath, linterOptions) == 0);
    }

    BOOST_FIXTURE_TEST_CASE(linterReturn1, initParseAndCallLinterTests)
    {
        linterName = CURRENT_BINARY_DIR"/mockReturn1";
        BOOST_CHECK(callLinter(linterName, yamlFilePath, linterOptions) == 1);
    }

    BOOST_FIXTURE_TEST_CASE(linterReturn2, initParseAndCallLinterTests)
    {
        linterName = CURRENT_BINARY_DIR"/mockReturn2";
        BOOST_CHECK(callLinter(linterName, yamlFilePath, linterOptions) == 2);
    }

    BOOST_FIXTURE_TEST_CASE(linterReturn0AndYamlExists, initParseAndCallLinterTests)
    {
        linterName = CURRENT_BINARY_DIR"/mockReturn0";
        yamlFilePath = "test.yaml";
        BOOST_CHECK(callLinter(linterName, yamlFilePath, linterOptions) == 0);
    }

    BOOST_FIXTURE_TEST_CASE(linterReturn0AndLinterOptionsExists, initParseAndCallLinterTests)
    {
        linterName = CURRENT_BINARY_DIR"/mockReturn0";
        linterOptions = "param1 param2";
        BOOST_CHECK(callLinter(linterName, yamlFilePath, linterOptions) == 0);
    }

    BOOST_FIXTURE_TEST_CASE(linterReturn0YamFileAndLinterOptionsExists, initParseAndCallLinterTests)
    {
        linterName = CURRENT_BINARY_DIR"/mockReturn0";
        std::cout << linterName;
        yamlFilePath = "file.yaml";
        linterOptions = "param1 param2";
        BOOST_CHECK(callLinter(linterName, yamlFilePath, linterOptions) == 0);
    }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(TestAddDocLinkToYamlFile)
    BOOST_FIXTURE_TEST_CASE(linterAndYamlFileNotExist, initAddDocToYamlFileTests)
    {
        linterName = "SomeNotExistentProgram";
        yamlFilePath = "SomeNotExistentFile";
        BOOST_CHECK(!addDocLinkToYAMLFile(linterName, yamlFilePath));
        for (const auto & contourFile : std::filesystem::directory_iterator(CURRENT_SOURCE_DIR"/../")) {
            BOOST_CHECK(CURRENT_SOURCE_DIR"/../linterYamlWithDocLink.yaml" != contourFile.path());
        }
    }

    BOOST_FIXTURE_TEST_CASE(linterNotExistsYamlEmpty, initAddDocToYamlFileTests)
    {
        linterName = "SomeNotExistentProgram";
        BOOST_CHECK(!addDocLinkToYAMLFile(linterName, yamlFilePath));
        for (const auto & contourFile : std::filesystem::directory_iterator(CURRENT_SOURCE_DIR"/../")) {
            BOOST_CHECK(CURRENT_SOURCE_DIR"/../linterYamlWithDocLink.yaml" != contourFile.path());
        }
    }

    BOOST_FIXTURE_TEST_CASE(ClangTidyYamlEmpty, initAddDocToYamlFileTests)
    {
        linterName = "clang-tidy";
        BOOST_CHECK(!addDocLinkToYAMLFile(linterName, yamlFilePath));
        for (const auto & contourFile : std::filesystem::directory_iterator(CURRENT_SOURCE_DIR"/../")) {
            BOOST_CHECK(CURRENT_SOURCE_DIR"/../linterYamlWithDocLink.yaml" != contourFile.path());
        }
    }

    BOOST_FIXTURE_TEST_CASE(ClazyStandaloneYamlEmpty, initAddDocToYamlFileTests)
    {
        linterName = "clazy-standalone";
        BOOST_CHECK(!addDocLinkToYAMLFile(linterName, yamlFilePath));
        for (const auto & contourFile : std::filesystem::directory_iterator(CURRENT_SOURCE_DIR"/../")) {
            BOOST_CHECK(CURRENT_SOURCE_DIR"/../linterYamlWithDocLink.yaml" != contourFile.path());
        }
    }

    BOOST_FIXTURE_TEST_CASE(mockProgrammYamlExists, initAddDocToYamlFileTests)
    {
        linterName = "mockProgramm";
        yamlFilePath = CURRENT_SOURCE_DIR"/yamlFiles/clazy.yaml";
        BOOST_CHECK(addDocLinkToYAMLFile(linterName, yamlFilePath));
        bool isYamlFileExists = false;
        for (const auto & contourFile : std::filesystem::directory_iterator(CURRENT_SOURCE_DIR"/../")) {
            if(CURRENT_SOURCE_DIR"/../linterYamlWithDocLink.yaml" == contourFile.path()) {
                isYamlFileExists = true;
            }
        }
        BOOST_REQUIRE(isYamlFileExists);

        YAML::Node yamlNode;
        yamlNode = YAML::LoadFile(CURRENT_SOURCE_DIR"/../linterYamlWithDocLink.yaml");
        for (auto it: yamlNode["Diagnostics"]) {
            std::ostringstream ossDocLink;
            ossDocLink << it["Documentation link"];
            BOOST_CHECK(ossDocLink.str() == "\"\"");
        }
    }

    BOOST_FIXTURE_TEST_CASE(ClangTidyYamlExists, initAddDocToYamlFileTests)
    {
        linterName = "clang-tidy";
        yamlFilePath = CURRENT_SOURCE_DIR"/yamlFiles/clangTidy.yaml";
        BOOST_CHECK(addDocLinkToYAMLFile(linterName, yamlFilePath));
        bool isYamlFileExists = false;
        for (const auto & contourFile : std::filesystem::directory_iterator(CURRENT_SOURCE_DIR"/../")) {
            if(CURRENT_SOURCE_DIR"/../linterYamlWithDocLink.yaml" == contourFile.path()) {
                isYamlFileExists = true;
            }
        }
        BOOST_CHECK(isYamlFileExists);

        YAML::Node yamlNode;
        yamlNode = YAML::LoadFile(CURRENT_SOURCE_DIR"/../linterYamlWithDocLink.yaml");
        for (auto it: yamlNode["Diagnostics"]) {
            std::ostringstream ossDocLink;
            ossDocLink << it["Documentation link"];
            std::ostringstream ossToCompare;
            ossToCompare << "https://clang.llvm.org/extra/clang-tidy/checks/" << it["DiagnosticName"] << ".html";
            BOOST_CHECK(ossDocLink.str() == ossToCompare.str());
        }
    }

    BOOST_FIXTURE_TEST_CASE(ClazyStandaloneYamlExists, initAddDocToYamlFileTests)
    {
        linterName = "clazy-standalone";
        yamlFilePath = CURRENT_SOURCE_DIR"/yamlFiles/clazy.yaml";
        BOOST_CHECK(addDocLinkToYAMLFile(linterName, yamlFilePath));
        bool isYamlFileExists = false;
        for (const auto & contourFile : std::filesystem::directory_iterator(CURRENT_SOURCE_DIR"/../")) {
            if(CURRENT_SOURCE_DIR"/../linterYamlWithDocLink.yaml" == contourFile.path()) {
                isYamlFileExists = true;
            }
        }
        BOOST_CHECK(isYamlFileExists);

        YAML::Node yamlNode;
        yamlNode = YAML::LoadFile(CURRENT_SOURCE_DIR"/../linterYamlWithDocLink.yaml");
        for (auto it: yamlNode["Diagnostics"]) {
            std::ostringstream ossDocLink;
            ossDocLink << it["Documentation link"];
            std::ostringstream tempOss;
            tempOss << it["DiagnosticName"];
            std::ostringstream ossToCompare;
            ossToCompare << "https://github.com/KDE/clazy/blob/master/docs/checks/README-";
            // substr() from 6 to size() for skipping "clazy-" in DiagnosticName
            ossToCompare << tempOss.str().substr(6, tempOss.str().size()) << ".md";
            BOOST_CHECK(ossDocLink.str() == ossToCompare.str());
        }
    }
BOOST_AUTO_TEST_SUITE_END()
