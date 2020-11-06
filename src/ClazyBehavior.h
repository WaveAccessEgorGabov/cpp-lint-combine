#pragma once

#include "LinterBehaviorItf.h"

namespace LintCombine {
    class ClazyBehavior final : public LinterBehaviorItf {
    public:
        std::string convertLinterOutput( std::string && linterOutputPart,
                                         const ReadLinterOutputFrom readFrom ) override;

    private:
        std::string m_stdoutBuffer;
        std::string m_stderrBuffer;
    };
}
