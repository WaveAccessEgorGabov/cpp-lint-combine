#pragma once

#include "LinterItf.h""

namespace LintCombine {

    struct PrepareCmdLineItf {
        virtual ~PrepareCmdLineItf() = default;

        virtual stringVector transform( stringVector commandLine ) = 0;

        virtual std::vector< Diagnostic > diagnostics() = 0;
    };
}
