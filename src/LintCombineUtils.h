#pragma once

#include "LinterItf.h"

namespace LintCombine {
    stringVector cmdLineToSTLContainer( int argc, char ** argv );

    void fixHyphensInCmdLine( stringVector & cmdLine );

    static stringVector prepareDiagnostics( const stringVector & cmdLine,
                                     const std::vector< Diagnostic > & diagnostics );

    void printDiagnostics( const stringVector & cmdLine,
                           const std::vector< Diagnostic > & diagnostics );
}




