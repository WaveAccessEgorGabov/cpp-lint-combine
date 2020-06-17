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

        std::vector < BaseLinterOptions * > m_lintersOptions;
        std::vector < std::string > m_unrecognizedCollection;
        std::string m_pathToCommonYaml;
        std::string m_pathToWorkDir;
        std::string m_clazyChecks;
    };

    stringVector moveCommandLineToSTLContainer( int argc, char ** argv );
}


