#pragma once

#include "LinterBehaviorItf.h"

namespace LintCombine {
    class ClangTidyBehavior final : public LinterBehaviorItf {
    public:
        std::string convertLinterOutput( std::string && linterOutputPart,
                                         const ReadLinterOutputFrom ) override;
    };
}
