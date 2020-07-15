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
    // TODO: Print diagnostics after PrepareCmdLine here
    if( cmdLine.empty() ) { return 1; }

    LintCombine::LinterCombine combine( cmdLine );

    // TODO: Do we realy need to write into stderr warnings? Or we can make it through diagnostics
    combine.callLinter();
    const auto combineCallReturnCode = combine.waitLinter();
    if( combineCallReturnCode == 2 ) {
        std::cerr << "some linters are failed" << std::endl;
    }
    if( combineCallReturnCode == 3 ) {
        std::cerr << "all linters are failed" << std::endl;
    }
    const auto callTotals = combine.updateYaml();
    if( callTotals.failNum ) {
        std::cout << "Updating " << callTotals.failNum << " yaml-files was failed" << std::endl;
    }
    if( combine.getYamlPath().empty() ) {
        // gereral yaml isn't created
    }

    return 0;
}
