#pragma once

#include "LinterItf.h"

namespace LintCombine {

    class DiagnosticWorker {

    public:
        void printDiagnostics();

    private:
        stringVector prepareOutput();
    };
}
