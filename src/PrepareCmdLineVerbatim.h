#pragma once

#include "PrepareCmdLineItf.h"

namespace LintCombine {

    class PrepareCmdLineVerbatim final : public PrepareCmdLineItf { 

    public:
        stringVector transform( stringVector commandLine ) override;
            
        std::vector< Diagnostic > diagnostics() override;

    private:
        std::vector< Diagnostic > m_diagnostics;
    };
}

