#pragma once

#include "CallTotals.h"
#include "Diagnostic.h"

#include <vector>

namespace LintCombine {
    using stringVector = std::vector < std::string >;

    class LinterItf {

    public:
        virtual ~LinterItf() = default;

        virtual void callLinter() = 0;

        virtual int waitLinter() = 0;

        virtual CallTotals updateYaml() = 0;

        virtual const std::string & getYamlPath() = 0;

        virtual std::vector< Diagnostic > diagnostics() = 0;
    };
}

