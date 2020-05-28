#pragma once

#include "LinterItf.h"

namespace LintCombine {
    void prepareCommandLineForReSharper( stringVector & commandLineSTL );

    void moveCommandLineToSTLContainer( stringVector & commandLineSTL, int argc, char ** argv );
}


