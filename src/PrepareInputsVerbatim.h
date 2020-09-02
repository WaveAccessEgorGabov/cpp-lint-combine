#pragma once

#include "PrepareInputsItf.h"

namespace LintCombine {

    class PrepareInputsVerbatim final : public PrepareInputsItf {

    public:
        stringVector transformCmdLine( stringVector cmdLineVal ) override;

        void transformFiles() override {}

        std::vector< Diagnostic > diagnostics() override;

    private:
        bool validateLinters();

        bool validateCombinedYamlPath();

        stringVector m_cmdLine;
        std::vector< Diagnostic > m_diagnostics;
    };
}

