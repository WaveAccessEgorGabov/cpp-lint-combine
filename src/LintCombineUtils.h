#pragma once

#include "LinterItf.h"

namespace LintCombine {
    stringVector moveCmdLineIntoSTLContainer( int argc, char ** argv );

    void fixHyphensInCmdLine( stringVector & cmdLine );
}




