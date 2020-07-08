#pragma once

#include "LinterItf.h"
#include "PrepareCmdLineItf.h"

namespace LintCombine {
    stringVector cmdLineToSTLContainer( int argc, char ** argv );

    void fixHyphensInCmdLine( stringVector & cmdLine );

    static stringVector prepareDiagnostics( const stringVector & cmdLine,
                                     const std::vector< PrepareCmdLineItf::Diagnostic > & diagnostics );

    void printDiagnostics( const stringVector & cmdLine,
                           const std::vector< PrepareCmdLineItf::Diagnostic > & diagnostics );
}




