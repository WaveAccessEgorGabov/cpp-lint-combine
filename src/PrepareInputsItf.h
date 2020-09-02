#pragma once

#include "LinterItf.h"

namespace LintCombine {

    struct PrepareInputsItf {
        virtual ~PrepareInputsItf() = default;

        virtual stringVector transformCmdLine( const stringVector & commandLine ) = 0;

        virtual void transformFiles() = 0;

        virtual std::vector< Diagnostic > diagnostics() = 0;
    };
}
