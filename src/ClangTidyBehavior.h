#pragma once

#include "LinterBehaviorItf.h"

namespace LintCombine {
    class ClangTidyBehavior : public LinterBehaviorItf {
    public:
        std::string convertLinterOutput( std::string && linterOutputPart ) override;
    };
}