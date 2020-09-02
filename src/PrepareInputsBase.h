#pragma once

#include "PrepareInputsItf.h"

#include <boost/predef.h>

#include <memory>

namespace LintCombine {

    class PrepareInputsBase : public PrepareInputsItf {

    public:
        stringVector transformCmdLine( const stringVector & cmdLineVal ) override;

        void transformFiles() override {}

        std::vector< Diagnostic > diagnostics() override;

    private:
        bool parseSourceCmdLine();

        bool validateParsedData();

        void checkIsOptionsValueInit( const std::string & optionName,
            const std::string & option );

        bool initLinters();

        void initCmdLine();

        void initCommonOptions();

    protected:
        virtual void specifyTargetArch() {}

        void addOptionToLinterByName( const std::string & name,
            const std::string & option );

        void addOptionToAllLinters( const std::string & option );

        static std::string
            optionValueToQuotes( const std::string & optionName,
            const std::string & optionNameWithValue );

        virtual void appendLintersOptionToCmdLine();

        struct LinterOptionsBase {
            std::string name;
            stringVector options;
        };

        struct ClangTidyOptions : LinterOptionsBase {
            explicit ClangTidyOptions( const std::string & pathToWorkDir ) {
                name = "clang-tidy";
                options.emplace_back( "--sub-linter=clang-tidy" );
                if( !pathToWorkDir.empty() ) {
                    options.emplace_back( "-p=" + pathToWorkDir );
                    if constexpr( BOOST_OS_WINDOWS ) {
                        options.emplace_back( "--export-fixes=" + pathToWorkDir +
                            "\\diagnosticsClangTidy.yaml" );
                    }
                    if constexpr( BOOST_OS_LINUX ) {
                        options.emplace_back( "--export-fixes=" + pathToWorkDir +
                            "/diagnosticsClangTidy.yaml" );
                    }
                }
            }
        };

        struct ClazyOptions : LinterOptionsBase {
            explicit ClazyOptions( const std::string & pathToWorkDir,
                const std::string & checks,
                const std::vector< std::string > & clangExtraArgs ) {
                name = "clazy";
                options.emplace_back( "--sub-linter=clazy" );
                if( !pathToWorkDir.empty() ) {
                    options.emplace_back( "-p=" + pathToWorkDir );
                    if constexpr( BOOST_OS_WINDOWS ) {
                        options.emplace_back( "--export-fixes=" + pathToWorkDir +
                            "\\diagnosticsClazy.yaml" );
                    }
                    if constexpr( BOOST_OS_LINUX ) {
                        options.emplace_back( "--export-fixes=" + pathToWorkDir +
                            "/diagnosticsClazy.yaml" );
                    }

                }
                if( !checks.empty() ) {
                    options.emplace_back( "--checks=" + checks );
                }
                if( !clangExtraArgs.empty() ) {
                    for( const auto & it : clangExtraArgs ) {
                        if( !it.empty() ) {
                            options.emplace_back( "--extra-arg=" + it );
                        }
                    }
                }
            }
        };

    private:
        std::string  m_pathToCombinedYaml;
        std::string  m_clazyChecks;
        std::string  m_clangExtraArgs;
        stringVector m_lintersNames;
        std::string  m_sourceCL;

    protected:
        std::string  pathToWorkDir;
        stringVector cmdLine;
        std::vector< Diagnostic > diagnosticsList;
        stringVector unrecognizedCollection;
        std::vector< std::unique_ptr< LinterOptionsBase > > lintersOptions;
    };
}
