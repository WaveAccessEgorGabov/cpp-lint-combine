#pragma once

#include "PrepareCmdLineItf.h"

namespace LintCombine {

    class PrepareCmdLineVerbatim final : public PrepareCmdLineItf {

    public:
        stringVector transform( stringVector cmdLineVal ) override;

        std::vector< Diagnostic > diagnostics() override;

    private:
        bool parseSourceCmdLine();

        bool validateParsedData();

        bool validateLinters();

        bool validateGeneralYamlPath();

        stringVector m_cmdLine;
        std::string m_pathToGeneralYaml;
        stringVector m_lintersNames;
        std::vector< Diagnostic > m_diagnostics;
    };
}

