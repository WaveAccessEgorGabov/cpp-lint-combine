#pragma once

#include "PrepareCmdLineItf.h"

namespace LintCombine {

    class PrepareCmdLineVerbatim final : public PrepareCmdLineItf {

    public:
        stringVector transform( stringVector commandLine ) override;

        std::vector< Diagnostic > diagnostics() override;

    private:
        bool parseSourceCmdLine();

        bool validateParsedData();

        bool checkYamlPathForCorrectness();

        stringVector m_lintersNames;
        std::vector< Diagnostic > m_diagnostics;
    };
}

