#pragma once

#include "LinterBehaviorItf.h"

namespace LintCombine {
    class ClazyBehavior final : public LinterBehaviorItf {
    public:
        std::streamsize convertLinterOutput( std::string & linterOutputPart ) override;

    private:
        std::string m_stdoutBuffer;
        std::string m_stderrBuffer;
    };
}
