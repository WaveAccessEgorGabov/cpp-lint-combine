#pragma once

#include <string>

namespace LintCombine {
    enum class ReadLinterOutputFrom { Stdout, Stderr };

    class LinterBehaviorItf {
    public:
        virtual ~LinterBehaviorItf() = default;

        virtual std::string convertLinterOutput( std::string && linterOutputPart,
                                                 const ReadLinterOutputFrom readFrom ) = 0;
    };
}
