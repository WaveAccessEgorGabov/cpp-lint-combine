#pragma once

#include "LinterItf.h"

namespace LintCombine {
    struct BaseLinterOptions {
        std::vector < std::string > options;
    };

    struct ClangTidyOptions : BaseLinterOptions {
        explicit ClangTidyOptions( const std::string & pathToWorkDir ) {
            options.emplace_back( "--sub-linter=clang-tidy" );
            options.emplace_back( "-p=" + pathToWorkDir );
            options.emplace_back( "--export-fixes=" + pathToWorkDir + "\\diagnosticsClangTidy.yaml" );
        }
    };

    struct ClazyOptions : BaseLinterOptions {
        explicit ClazyOptions( const std::string & pathToWorkDir,
                               const std::string & checks,
                               const std::string & clangExtraArgs ) {
            options.emplace_back( "--sub-linter=clazy" );
            options.emplace_back( "-p=" + pathToWorkDir );
            options.emplace_back( "--export-fixes=" + pathToWorkDir + "\\diagnosticsClazy.yaml" );
            if( !checks.empty () ) {
                options.emplace_back ( "--checks=" + checks );
            }
            if( !clangExtraArgs.empty () ) {
                options.emplace_back ( "--extra-arg=" + clangExtraArgs );
            }
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

        std::vector < BaseLinterOptions * > m_lintersOptions;
        stringVector m_unrecognizedCollection;
        std::string m_pathToCommonYaml;
        std::string m_pathToWorkDir;
        std::string m_clazyChecks;
        std::string m_clangExtraArgs;
        std::string m_clangTidyExtraChecks;
    };

    stringVector moveCommandLineToSTLContainer( int argc, char ** argv );
}


