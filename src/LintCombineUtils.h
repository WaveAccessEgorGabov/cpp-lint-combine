#pragma once

#include "LinterItf.h"

#include <sstream>
#include <iostream>

namespace LintCombine {
    class CommandLinePreparer {

    public:
        CommandLinePreparer( stringVector & commandLine, std::string && toolName );

        bool getIsErrorWhilePrepareOccur () const { return isErrorWhilePrepareOccur; }

    private:
        class Logger {
        public:
            Logger( Logger & ) = delete;

            Logger( Logger && ) = delete;

            Logger & operator=( Logger const & ) = delete;

            Logger & operator=( Logger const && ) = delete;

            static Logger & getInstance () {
                static Logger instance;
                return instance;
            }

            void printLog() const {
                if( !warnings.str().empty () ) {
                    std::cerr << "Warnings:" << std::endl;
                    std::cerr << warnings.str() << std::endl;
                }
                if( !errors.str().empty () ) {
                    std::cerr << "Errors:" << std::endl;
                    std::cerr << errors.str() << std::endl;
                }
            }

            std::ostringstream warnings;
            std::ostringstream errors;

        private:
            Logger() = default;
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

        std::vector < BaseLinterOptions * > m_lintersOptions;
        stringVector m_unrecognizedCollection;
        stringVector m_lintersName;
        std::string m_pathToCommonYaml;
        std::string m_pathToWorkDir;
        std::string m_clazyChecks;
        std::string m_clangExtraArgs;
        bool isErrorWhilePrepareOccur = false;
    };

    stringVector moveCommandLineToSTLContainer( int argc, char ** argv );
}


