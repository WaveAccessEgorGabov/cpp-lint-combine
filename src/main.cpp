#include "LinterCombine.h"
#include "LintCombineUtils.h"
#include "IdeTraitsFactory.h"
#include "DiagnosticWorker.h"

int main( int argc, char * argv[] ) {
    LintCombine::stringVector cmdLine = LintCombine::moveCmdLineIntoSTLContainer( argc, argv );
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareInputs = ideTraitsFactory.getPrepareCmdLineInstance( cmdLine );
    const LintCombine::DiagnosticWorker diagnosticWorker( cmdLine, argc == 1 );
    cmdLine = prepareInputs->transformCmdLine( cmdLine );
    prepareInputs->transformFiles();

    if( diagnosticWorker.printDiagnostics( prepareInputs->diagnostics() ) ) {
        return 0;
    }
    if( cmdLine.empty() ) {
        return 1;
    }

    std::unique_ptr<LintCombine::LinterCombine > pCombine;
    try {
        pCombine = std::make_unique<LintCombine::LinterCombine>( cmdLine );
    }
    catch( const LintCombine::Exception & ex ) {
        diagnosticWorker.printDiagnostics( ex.diagnostics() );
        return 1;
    }

    pCombine->callLinter();
    const auto callReturnCode = pCombine->waitLinter();
    if( callReturnCode == 3 ) {
        diagnosticWorker.printDiagnostics( pCombine->diagnostics() );
        return callReturnCode;
    }

    if( ideTraitsFactory.getIdeBehaviorInstance() &&
        ideTraitsFactory.getIdeBehaviorInstance()->isYamlContainsDocLink() ) {
        const auto callTotals = pCombine->updateYaml();
        if( callTotals.failNum == pCombine->numLinters() ) {
            diagnosticWorker.printDiagnostics( pCombine->diagnostics() );
            return 1;
        }
    }

    if( pCombine->getYamlPath().empty() ) {
        diagnosticWorker.printDiagnostics( pCombine->diagnostics() );
        return 1;
    }
    diagnosticWorker.printDiagnostics( pCombine->diagnostics() );

    return 0;
}
