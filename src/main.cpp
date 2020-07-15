#include "LinterCombine.h"
#include "LintCombineUtils.h"
#include "PrepareCmdLineFactory.h"
#include "DiagnosticWorker.h"

int main( int argc, char * argv[] ) {
    LintCombine::stringVector cmdLine = LintCombine::cmdLineToSTLContainer( argc, argv );
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine );
    const LintCombine::DiagnosticWorker diagnosticWorker( cmdLine );
    cmdLine = prepareCmdLine->transform( cmdLine );
    
    if( diagnosticWorker.printDiagnostics( prepareCmdLine->diagnostics() ) ) {
        return 0;
    }
    if( cmdLine.empty() ) {
        return 1;
    }

    LintCombine::LinterCombine combine( cmdLine );
    if( combine.getIsErrorOccur() ) {
        diagnosticWorker.printDiagnostics( combine.diagnostics() );
        return 1;
    }

    combine.callLinter();
    const auto ñallReturnCode = combine.waitLinter();
    if( ñallReturnCode == 3 ) {
        diagnosticWorker.printDiagnostics( combine.diagnostics() );
        return 1;
    }

    const auto callTotals = combine.updateYaml();
    if( callTotals.failNum == combine.numLinters() ) {
        diagnosticWorker.printDiagnostics( combine.diagnostics() );
        return 1;
    }

    if( combine.getYamlPath().empty() ) {
        diagnosticWorker.printDiagnostics( combine.diagnostics() );
        return 1;
    }
    diagnosticWorker.printDiagnostics( combine.diagnostics() );

    return 0;
}
