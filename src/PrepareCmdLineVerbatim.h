#pragma once

#include "PrepareCmdLineItf.h"

namespace LintCombine {

    class PrepareCmdLineVerbatim final : public PrepareCmdLineItf {

    public:
        stringVector transform( stringVector cmdLineVal ) override;

        std::vector< Diagnostic > diagnostics() override;

    private:
        bool validateLinters();

        void validateGeneralYamlPath();

        stringVector m_cmdLine;
        std::vector< Diagnostic > m_diagnostics;
    };
}

