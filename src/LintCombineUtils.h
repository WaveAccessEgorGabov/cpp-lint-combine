#pragma once

#include "LinterItf.h"

namespace LintCombine {
    struct BaseLinterOptions {
        std::vector < std::string > options;
    };

    struct ClangTidyOptions : public BaseLinterOptions {
        explicit ClangTidyOptions(const std::string & pathToWorkDir) {
            options.emplace_back("--sub-linter=clang-tidy");
            options.emplace_back("-p=" + pathToWorkDir);
            options.emplace_back("--export-fixes=" + pathToWorkDir + "\\diagnosticsClangTidy.yaml");
        }
    };

    struct ClazyOptions : public BaseLinterOptions {
        explicit ClazyOptions(const std::string& pathToWorkDir) {
            options.emplace_back("--sub-linter=clazy");
            options.emplace_back("-p=" + pathToWorkDir);
            options.emplace_back("--export-fixes=" + pathToWorkDir + "\\diagnosticsClazy.yaml");
        }
    };

    class CommandLineOptions {
    public:
        std::string& getPathToWorkDirRef() { return pathToWorkDir; }
        std::string& getPathToCommonYamlRef() { return pathToCommonYaml; }
        std::vector < std::string >& getUnrecognizedCollectionRef() { return unrecognizedCollection; }
        void initCommandLine(std::vector < std::string >& commandLine);
    private:
        static std::string optionValueToQuotes(const std::string& optionName, const std::string& optionNameWithValue);
        void addOptionToClangTidy(const std::string& option);
        void addOptionToAllLinters(const std::string& option);
        void appendLintersOptionToCommandLine(std::vector < std::string >& commandLine);
        void initUnrecognizedOptions();
        void initLintCombineOptions(std::vector < std::string >& commandLine) const;
        std::vector < BaseLinterOptions* > lintersOptions;
        std::vector < std::string > unrecognizedCollection;
        std::string pathToCommonYaml;
        std::string pathToWorkDir;
    };

    void prepareCommandLineForReSharper( stringVector & commandLine );

    void moveCommandLineToSTLContainer( stringVector & commandLineSTL, int argc, char ** argv );
}


