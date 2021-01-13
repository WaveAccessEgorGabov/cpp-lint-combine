#pragma once

#include "PrepareInputsItf.h"

#include <boost/predef.h>

#include <filesystem>
#include <memory>

namespace LintCombine {
    class PrepareInputsBase : public PrepareInputsItf {
    public:
        StringVector transformCmdLine( const StringVector & cmdLineVal ) override;

        void transformFiles() override {}

        std::vector< Diagnostic > diagnostics() const override;

        bool isCalledExplicitly() override {
            return calledExplicitly;
        }

    protected:
        virtual void specifyTargetArch() {}

        void addOptionToLinterByName( const std::string & name, const std::string & option );

        void addOptionToAllLinters( const std::string & option );

        static std::string optionValueToQuotes( const std::string & optionName,
                                                const std::string & optionNameWithValue );

        virtual void appendLintersOptionToCmdLine();

        struct LinterOptionsBase {
            std::string name;
            StringVector options;
        };

        struct ClangTidyOptions : LinterOptionsBase {
            explicit ClangTidyOptions( const std::string & pathToWorkDirVal,
                                       const bool linterHasYaml ) {
                name = "clang-tidy";
                options.emplace_back( "--sub-linter=clang-tidy" );
                if( !pathToWorkDirVal.empty() ) {
                    options.emplace_back( "-p=" + pathToWorkDirVal );
                }
                if( linterHasYaml ) {
                    if constexpr( BOOST_OS_WINDOWS ) {
                        options.emplace_back(
                            "--export-fixes=" + std::filesystem::temp_directory_path().string()
                            + "\\diagnosticsClangTidy.yaml" );
                    }
                    else {
                        options.emplace_back(
                            "--export-fixes=" + std::filesystem::temp_directory_path().string()
                            + "/diagnosticsClangTidy.yaml" );
                    }
                }
            }
        };

        struct ClazyOptions : LinterOptionsBase {
            explicit ClazyOptions( const std::string & pathToWorkDirVal,
                                   const bool linterHasYaml, const std::string & checks,
                                   const std::vector< std::string > & clangExtraArgs ) {
                name = "clazy";
                options.emplace_back( "--sub-linter=clazy" );
                if( !pathToWorkDirVal.empty() ) {
                    options.emplace_back( "-p=" + pathToWorkDirVal );
                }
                if( linterHasYaml ) {
                    if constexpr( BOOST_OS_WINDOWS ) {
                        options.emplace_back(
                            "--export-fixes=" + std::filesystem::temp_directory_path().string()
                            + "\\diagnosticsClazy.yaml" );
                    }
                    else {
                        options.emplace_back(
                            "--export-fixes=" + std::filesystem::temp_directory_path().string()
                            + "/diagnosticsClazy.yaml" );
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
        bool parseSourceCmdLine();

        bool validateParsedData();

        bool initLinters();

        void initCmdLine();

        void initCommonOptions();

    protected:
        std::string  pathToWorkDir;
        StringVector cmdLine;
        StringVector unrecognizedCollection;
        std::vector< std::unique_ptr< LinterOptionsBase > > lintersOptions;
        std::vector< Diagnostic > m_diagnostics;
        bool calledExplicitly = false;

    private:
        std::string  m_pathToCombinedYaml;
        std::string  m_clazyChecks;
        std::string  m_clangExtraArgs;
        StringVector m_lintersNames;
        std::string  m_sourceCmdLine;
    };
}
