#pragma once

#include "PrepareCmdLineItf.h"

namespace LintCombine {

    class PrepareCmdLineBase : public PrepareCmdLineItf {

    public:
        stringVector transform( stringVector cmdLineVal ) override;

        std::vector< Diagnostic > diagnostics() override;

    private:
        bool parseSourceCmdLine();

        bool validateParsedData();

        void checkIsOptionsValueInit( const std::string& optionName,
                                      const std::string& option);

        bool initLinters();

        void realeaseClassField();

    protected:
        virtual void appendOptionsToSpecificIDE() = 0;

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
                    options.emplace_back( "--export-fixes=" + pathToWorkDir + "\\diagnosticsClangTidy.yaml" );
                }
            }
        };

        struct ClazyOptions : LinterOptionsBase {
            explicit ClazyOptions( const std::string & pathToWorkDir,
                                    const std::string & checks,
                                    std::vector < std::string > && clangExtraArgs ) {
                name = "clazy";
                options.emplace_back( "--sub-linter=clazy" );
                if( !pathToWorkDir.empty() ) {
                    options.emplace_back( "-p=" + pathToWorkDir );
                    options.emplace_back( "--export-fixes=" + pathToWorkDir + "\\diagnosticsClazy.yaml" );
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
        // TODO : something is private
        // TODO : may be not delet all class fields
        stringVector m_cmdLine;
        std::string  m_sourceCL;
        std::vector< Diagnostic > m_diagnostics;
        std::string  m_pathToGeneralYaml;
        std::string  m_pathToWorkDir;
        std::string  m_clazyChecks;
        std::string  m_clangExtraArgs;
        stringVector m_lintersNames;
        stringVector m_unrecognizedCollection;
        std::vector < LinterOptionsBase * > m_lintersOptions;
    };
}