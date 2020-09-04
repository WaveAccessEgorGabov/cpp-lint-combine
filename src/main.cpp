#include "LinterCombine.h" // for LintCombine::WaitLinterReturnCode
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

    if( diagnosticWorker.printDiagnostics( prepareInputs->diagnostics() ) || cmdLine.empty() ) {
        return 0;
    }

    std::unique_ptr< LintCombine::LinterItf > lintCombine;
    try {
        cmdLine.insert( cmdLine.begin(), "LinterCombine" );
        lintCombine = LintCombine::UsualLinterFactory::getInstance().createLinter( cmdLine );
    }
    catch( const LintCombine::Exception & ex ) {
        diagnosticWorker.printDiagnostics( ex.diagnostics() );
        return 1; // Error occurred in LinterCombine constructor.
    }

    lintCombine->callLinter();
    const auto callReturnCode = lintCombine->waitLinter();
    if( callReturnCode == LintCombine::AllLintersFailed ) {
        diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );
        if( ideTraitsFactory.getIdeBehaviorInstance() &&
            !ideTraitsFactory.getIdeBehaviorInstance()->isLinterExitCodeTolerant() ) {
            return callReturnCode; // All linters failed.
        }
    }

    if( ideTraitsFactory.getIdeBehaviorInstance() &&
        ideTraitsFactory.getIdeBehaviorInstance()->mayYamlFileContainDocLink() ) {
        const auto callTotals = lintCombine->updateYaml();
        if( !callTotals.successNum ) {
            diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );
            return 2; // Error while updating YAML-file
        }
    }

    if( lintCombine->getYamlPath().empty() ) {
        diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );
        return 4; // Error while put diagnostics into result YAML-file.
    }
    diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );

    return 0;
}
