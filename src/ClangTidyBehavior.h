#pragma once

#include "LinterBehaviorItf.h"

namespace LintCombine {
    class ClangTidyBehavior final : public LinterBehaviorItf {
    public:
        std::streamsize convertLinterOutput( std::string & linterOutputPart ) override;
    };
}
