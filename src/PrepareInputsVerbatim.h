#pragma once

#include "PrepareInputsItf.h"

namespace LintCombine {

    class PrepareInputsVerbatim final : public PrepareInputsItf {

    public:
        stringVector transformCmdLine( const stringVector & cmdLineVal ) override;

        void transformFiles() override {}

        std::vector< Diagnostic > diagnostics() const override;

    private:
        bool validateLinters();

        bool validateCombinedYamlPath();

        stringVector m_cmdLine;
        std::vector< Diagnostic > m_diagnostics;
    };
}
