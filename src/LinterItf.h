#pragma once

#include "CallTotals.h"
#include <string>
#include <vector>

namespace LintCombine {
    using stringVector = std::vector < std::string >;
    using stringVectorConstRef = const stringVector &;

    class LinterItf {
    public:
        virtual ~LinterItf() = default;

        virtual void callLinter() = 0;

        virtual int waitLinter() = 0;

        virtual CallTotals updateYaml() const = 0;

        virtual const std::string & getYamlPath() = 0;
    };
}

