#pragma once

#include "LinterBehaviorItf.h"

namespace LintCombine {
    class ClazyBehavior : public LinterBehaviorItf {
    public:
        std::string convertLinterOutput( std::string && linterOutputPart ) override;
    };
}