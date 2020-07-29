#pragma once

#include "LinterItf.h"

namespace LintCombine {
    stringVector cmdLineToSTLContainer( int argc, char ** argv );

    void fixHyphensInCmdLine( stringVector & cmdLine );
}




