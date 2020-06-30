#pragma once

#include "LinterItf.h"

namespace LintCombine {

    class CommandLinePreparer {

    public:
        CommandLinePreparer( stringVector & commandLine, std::string && toolName );

        bool getIsErrorWhilePrepareOccur () const { return m_isErrorWhilePrepareOccur; }

    private:
        struct OutputRecord {
            std::string level;
            std::string text;
            unsigned int startPosInCL;
            unsigned int endPosInCL;

            OutputRecord( const std::string & levelInit, const std::string & textInit,
                          const unsigned int startPosInCLInit,
                          const unsigned int endPosInCLInit )
                : level( levelInit ), text( textInit ),
                  startPosInCL( startPosInCLInit ), endPosInCL( endPosInCLInit ) {}
        };

        struct BaseLinterOptions {
            std::string linterName;
            stringVector options;
        };

        struct ClangTidyOptions : BaseLinterOptions {
            explicit ClangTidyOptions ( const std::string & pathToWorkDir ) {
                linterName = "clang-tidy";
                options.emplace_back ( "--sub-linter=clang-tidy" );
                if( !pathToWorkDir.empty () ) {
                    options.emplace_back ( "-p=" + pathToWorkDir );
                    options.emplace_back ( "--export-fixes=" + pathToWorkDir + "\\diagnosticsClangTidy.yaml" );
                }
            }
        };

        struct ClazyOptions : BaseLinterOptions {
            explicit ClazyOptions ( const std::string & pathToWorkDir,
                                    const std::string & checks,
                                    std::vector < std::string > && clangExtraArgs ) {
                linterName = "clazy";
                options.emplace_back ( "--sub-linter=clazy" );
                if( !pathToWorkDir.empty () ) {
                    options.emplace_back ( "-p=" + pathToWorkDir );
                    options.emplace_back ( "--export-fixes=" + pathToWorkDir + "\\diagnosticsClazy.yaml" );
                }
                if( !checks.empty () && checks.find ( "--" ) == std::string::npos ) {
                    options.emplace_back ( "--checks=" + checks );
                }
                if( !clangExtraArgs.empty () ) {
                    for( const auto & it : clangExtraArgs ) {
                        if( !it.empty () && it.find ( "--" ) == std::string::npos ) {
                            options.emplace_back ( "--extra-arg=" + it );
                        }
                    }
                }
            }
        };

        void prepareCommandLineForReSharper( stringVector & commandLine );

        static std::string
        optionValueToQuotes( const std::string & optionName, const std::string & optionNameWithValue );

        void initCommandLine( stringVector & commandLine );

        void addOptionToClangTidy( const std::string & option );

        void addOptionToAllLinters( const std::string & option );

        void appendLintersOptionToCommandLine( stringVector & commandLine ) const;

        void initUnrecognizedOptions();

        void initLintCombineOptions( stringVector & commandLine ) const;

        stringVector prepareOutput() const;

        void printOutput() const;

        std::vector < BaseLinterOptions * > m_lintersOptions;
        std::vector < OutputRecord > m_output;
        stringVector m_unrecognizedCollection;
        stringVector m_lintersNames;
        std::string m_pathToGeneralYaml;
        std::string m_pathToWorkDir;
        std::string m_clazyChecks;
        std::string m_clangExtraArgs;
        std::string m_sourceCL;
        bool m_isErrorWhilePrepareOccur = false;
    };

    stringVector moveCommandLineToSTLContainer( int argc, char ** argv );
}


