#pragma once

#include "LinterBehaviorItf.h"

namespace LintCombine {
    class ClangTidyBehavior final : public LinterBehaviorItf {
    public:
        int convertLinterOutput( std::string & linterOutputPart ) override;
    };
}
