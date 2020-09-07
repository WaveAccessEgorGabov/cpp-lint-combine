#pragma once

#include "PrepareInputsItf.h"

#include <boost/predef.h>

#include <memory>

namespace LintCombine {

    class PrepareInputsBase : public PrepareInputsItf {

    public:
        stringVector transformCmdLine( const stringVector & cmdLineVal ) override;

        void transformFiles() override {}

        std::vector< Diagnostic > diagnostics() const override;

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
            explicit ClangTidyOptions( const std::string & pathToWorkDirVal ) {
                name = "clang-tidy";
                options.emplace_back( "--sub-linter=clang-tidy" );
                if( !pathToWorkDirVal.empty() ) {
                    options.emplace_back( "-p=" + pathToWorkDirVal );
                    if constexpr( BOOST_OS_WINDOWS ) {
                        options.emplace_back( "--export-fixes=" + pathToWorkDirVal +
                                              "\\diagnosticsClangTidy.yaml" );
                    }
                    if constexpr( BOOST_OS_LINUX ) {
                        options.emplace_back( "--export-fixes=" + pathToWorkDirVal +
                                              "/diagnosticsClangTidy.yaml" );
                    }
                }
            }
        };

        struct ClazyOptions : LinterOptionsBase {
            explicit ClazyOptions( const std::string & pathToWorkDirVal,
                                   const std::string & checks,
                                   const std::vector< std::string > & clangExtraArgs ) {
                name = "clazy";
                options.emplace_back( "--sub-linter=clazy" );
                if( !pathToWorkDirVal.empty() ) {
                    options.emplace_back( "-p=" + pathToWorkDirVal );
                    if constexpr( BOOST_OS_WINDOWS ) {
                        options.emplace_back(
                            "--export-fixes=" + pathToWorkDirVal + "\\diagnosticsClazy.yaml" );
                    }
                    if constexpr( BOOST_OS_LINUX ) {
                        options.emplace_back(
                            "--export-fixes=" + pathToWorkDirVal + "/diagnosticsClazy.yaml" );
                    }

                }
                if( !checks.empty() ) {
                    options.emplace_back( "--checks=" + checks );
                }
                if( !clangExtraArgs.empty() ) {
                    for( const auto & clangExtraArg : clangExtraArgs ) {
                        if( !clangExtraArg.empty() ) {
                            options.emplace_back( "--extra-arg=" + clangExtraArg );
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
        std::string  m_sourceCmdLine;
        std::vector< Diagnostic > m_diagnostics;

    protected:
        std::string  pathToWorkDir;
        stringVector cmdLine;
        stringVector unrecognizedCollection;
        std::vector< std::unique_ptr< LinterOptionsBase > > lintersOptions;
    };
}
