#include "LinterCombine.h"
#include "LintCombineUtils.h"
#include "PrepareCmdLineFactory.h"

#include <iostream>

// TODO: --help to Diagnostics into factory
// TODO: Fix main

int main( int argc, char * argv[] ) {
    LintCombine::stringVector cmdLine = LintCombine::cmdLineToSTLContainer( argc, argv );
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    cmdLine = prepareCmdLine->transform( cmdLine );
    // TODO: Print diagnostics from Preparer
    if( cmdLine.empty() ) {
        return 1;
    }

    LintCombine::LinterCombine combine( cmdLine );
    if( combine.getIsErrorOccur() ) {
        // TODO: Print diagnostics from Combine
        return 1;
    }

    combine.callLinter();
    const auto ñallReturnCode = combine.waitLinter();
    if( ñallReturnCode == 3 ) {
        // TODO: Print diagnostics from Combine 
        return 1;
    }
    const auto callTotals = combine.updateYaml();
    if( callTotals.failNum == combine.numLinters() ) {
        // TODO: Print diagnostics from Combine 
        return 1;
    }
    if( combine.getYamlPath().empty() ) {
        // TODO: Print diagnostics from Combine
        return 1;
    }
    // TODO: Print diagnostics from Combine

    return 0;
}
