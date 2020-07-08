#include "LinterCombine.h"
#include "LintCombineUtils.h"
#include "PrepareCmdLineFactory.h"

#include <iostream>

int main( int argc, char * argv[] ) {
    LintCombine::stringVector cmdLine = LintCombine::cmdLineToSTLContainer( argc, argv );
    LintCombine::fixHyphensInCmdLine( cmdLine );
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    const auto saveCmdLine = cmdLine;
    cmdLine = prepareCmdLine->transform( cmdLine );
    printDiagnostics( saveCmdLine, prepareCmdLine->diagnostics() );
    if( cmdLine.empty() ) {
        return 1;
    }

    try {
        LintCombine::LinterCombine linterCombine( cmdLine );
        if( linterCombine.printTextIfRequested() ) {
            return 0;
        }
        linterCombine.callLinter();
        const int linterReturnCode = linterCombine.waitLinter();
        if( linterReturnCode == 2 ) {
            std::cerr << "some linters are failed" << std::endl;
        }
        if( linterReturnCode == 3 ) {
            std::cerr << "all linters are failed" << std::endl;
        }
        const LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        if( callTotals.failNum ) {
            std::cout << "Updating " << callTotals.failNum << " yaml-files was failed" << std::endl;
        }
        linterCombine.getYamlPath();
    }
    catch( const std::logic_error & ex ) {
        std::cerr << "std::logic_error exception. What(): " << ex.what() << std::endl;
        return 1;
    }
    catch( const std::exception & ex ) {
        std::cerr << "Exception. What(): " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
