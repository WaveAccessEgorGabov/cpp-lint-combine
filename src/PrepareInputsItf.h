#pragma once

#include "LinterItf.h"

namespace LintCombine {
    struct PrepareInputsItf {
        virtual ~PrepareInputsItf() = default;

        virtual StringVector transformCmdLine( const StringVector & commandLine ) = 0;

        virtual void transformFiles() = 0;

        virtual std::vector< Diagnostic > diagnostics() const = 0;

        virtual bool isCalledExplicitly() const = 0;
    };
}
