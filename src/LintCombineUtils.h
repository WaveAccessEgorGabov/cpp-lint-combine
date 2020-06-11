#pragma once

#include "LinterItf.h"

namespace LintCombine {
    struct BaseLinterOptions {
        std::vector < std::string > options;
    };

    struct ClangTidyOptions : public BaseLinterOptions {
        explicit ClangTidyOptions( const std::string & pathToWorkDir ) {
            options.emplace_back( "--sub-linter=clang-tidy" );
            options.emplace_back( "-p=" + pathToWorkDir );
            options.emplace_back( "--export-fixes=" + pathToWorkDir + "\\diagnosticsClangTidy.yaml" );
        }
    };

    struct ClazyOptions : public BaseLinterOptions {
        explicit ClazyOptions( const std::string & pathToWorkDir,
                               const std::string & checks ) {
            options.emplace_back( "--sub-linter=clazy" );
            if( !checks.empty() ) {
                options.emplace_back( "-checks=" + checks );
            }
            options.emplace_back( "-p=" + pathToWorkDir );
            options.emplace_back( "--export-fixes=" + pathToWorkDir + "\\diagnosticsClazy.yaml" );
        }
    };

    class CommandLinePreparer {
    public:
        CommandLinePreparer( stringVector & commandLine, std::string && toolName );

    private:
        void prepareCommandLineForReSharper( stringVector & commandLine );

        static std::string
        optionValueToQuotes( const std::string & optionName, const std::string & optionNameWithValue );

        void initCommandLine( stringVector & commandLine );

        void addOptionToClangTidy( const std::string & option );

        void addOptionToAllLinters( const std::string & option );

        void appendLintersOptionToCommandLine( stringVector & commandLine ) const;

        void initUnrecognizedOptions();

        void initLintCombineOptions( stringVector & commandLine ) const;

        std::vector < BaseLinterOptions * > lintersOptions;
        std::vector < std::string > unrecognizedCollection;
        std::string pathToCommonYaml;
        std::string pathToWorkDir;
        std::string clazyChecks;
    };

    stringVector moveCommandLineToSTLContainer( int argc, char ** argv );
}


