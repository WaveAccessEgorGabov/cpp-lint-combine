#pragma once

#include "LinterItf.h"

namespace LintCombine {
    void prepareCommandLine( stringVectorConstRef commandLineSTL );

    char ** prepareCommandLine( int & argc, char ** argv );
}


