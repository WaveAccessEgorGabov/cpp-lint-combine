#pragma once

#include <string>

namespace LintCombine {
    class LinterBehaviorItf {
    public:
        virtual ~LinterBehaviorItf() = default;

        virtual std::streamsize convertLinterOutput( std::string & linterOutputPart ) = 0;
    };
}
