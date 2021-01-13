#pragma once

#include "PrepareInputsItf.h"

namespace LintCombine {
    class PrepareInputsVerbatim final : public PrepareInputsItf {
    public:
        StringVector transformCmdLine( const StringVector & cmdLineVal ) override;

        void transformFiles() override {}

        std::vector< Diagnostic > diagnostics() const override;

        bool isCalledExplicitly() const override {
            return false;
        }

    private:
        bool validateLinters();

        bool validateCombinedYamlPath();

        StringVector m_cmdLine;
        std::vector< Diagnostic > m_diagnostics;
    };
}
