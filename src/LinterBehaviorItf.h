#pragma once

#include <string>

namespace LintCombine {
    class LinterBehaviorItf {
    public:
        virtual ~LinterBehaviorItf() = default;

        virtual int convertLinterOutput( std::string & linterOutputPart ) = 0;
    };
}
