#include "LinterCombine.h"
#include "LintCombineUtils.h"
#include "IdeTraitsFactory.h"
#include "DiagnosticWorker.h"

int main( int argc, char * argv[] ) {
    LintCombine::stringVector cmdLine = LintCombine::moveCmdLineIntoSTLContainer( argc, argv );
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareInputs = ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    const LintCombine::DiagnosticWorker diagnosticWorker( cmdLine, argc == 1 );
    cmdLine = prepareInputs->transformCmdLine( cmdLine );
    prepareInputs->transformFiles();

    if( diagnosticWorker.printDiagnostics( prepareInputs->diagnostics() ) ) {
        return 0;
    }

    std::unique_ptr< LintCombine::LinterItf > pCombine;
    try {
        cmdLine.insert( cmdLine.begin(), "LinterCombine" );
        pCombine = LintCombine::UsualLinterFactory::getInstance().createLinter( cmdLine );
    }
    catch( const LintCombine::Exception & ex ) {
        diagnosticWorker.printDiagnostics( ex.diagnostics() );
        return 1; // Error occurred in LinterCombine constructor.
    }

    pCombine->callLinter();
    const auto callReturnCode = pCombine->waitLinter();
    if( callReturnCode == LintCombine::AllLintersFailed ) {
        diagnosticWorker.printDiagnostics( pCombine->diagnostics() );
        if( !ideTraitsFactory.getIdeBehaviorInstance()->isLinterExitCodeTolerant() ) {
            return callReturnCode; // All linters failed.
        }
    }

    if( ideTraitsFactory.getIdeBehaviorInstance() &&
        ideTraitsFactory.getIdeBehaviorInstance()->mayYamlContainsDocLink() ) {
        const auto callTotals = pCombine->updateYaml();
        if( !callTotals.successNum ) {
            diagnosticWorker.printDiagnostics( pCombine->diagnostics() );
            return 2; // Error while updating YAML-file
        }
    }

    if( pCombine->getYamlPath().empty() ) {
        diagnosticWorker.printDiagnostics( pCombine->diagnostics() );
        return 4; // Error while put diagnostics into result YAML-file.
    }
    diagnosticWorker.printDiagnostics( pCombine->diagnostics() );

    return 0;
}
