#include "LinterCombine.h"
#include "LintCombineUtils.h"
#include "PrepareCmdLineFactory.h"

#include <iostream>

// TODO: --help to Diagnostics into factory

int main( int argc, char * argv[] ) {
    LintCombine::stringVector cmdLine = LintCombine::cmdLineToSTLContainer( argc, argv );
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    cmdLine = prepareCmdLine->transform( cmdLine );
    // TODO: Print diagnostics after PrepareCmdLine here
    if( cmdLine.empty() ) { return 1; }

    LintCombine::LinterCombine * pCombine;
    try {
        LintCombine::LinterCombine combine( cmdLine );
        pCombine = &combine;
    }
    catch( const std::logic_error & ex ) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
    catch( const std::exception & ex ) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    // TODO: Do we realy need to write into stderr warnings? Or we can make it through diagnostics
    pCombine->callLinter();
    const auto combineCallReturnCode = pCombine->waitLinter();
    if( combineCallReturnCode == 2 ) {
        std::cerr << "some linters are failed" << std::endl;
    }
    if( combineCallReturnCode == 3 ) {
        std::cerr << "all linters are failed" << std::endl;
    }
    const LintCombine::CallTotals callTotals = pCombine->updateYaml();
    if( callTotals.failNum ) {
        std::cout << "Updating " << callTotals.failNum << " yaml-files was failed" << std::endl;
    }
    pCombine->getYamlPath();

    return 0;
}
