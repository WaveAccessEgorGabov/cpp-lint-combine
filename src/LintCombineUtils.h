#pragma once

#include "LinterItf.h"

namespace LintCombine {
    void prepareCommandLineForReSharper( stringVector & commandLine );

    void moveCommandLineToSTLContainer( stringVector & commandLineSTL, int argc, char ** argv );
}


