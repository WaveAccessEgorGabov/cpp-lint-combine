#include "LinterCombine.h" // for LintCombine::WaitLinterReturnCode
#include "LintCombineUtils.h"
#include "IdeTraitsFactory.h"
#include "DiagnosticWorker.h"

namespace LintCombine {
    enum ExitCode{ Success, FailedToConstructLinterCombine, FailedToUpdateYaml,
                   FailedToCallLinters, FailedToPutDiagsIntoYaml };
}

int main( int argc, char * argv[] ) {
    LintCombine::StringVector cmdLine = LintCombine::moveCmdLineIntoSTLContainer( argc, argv );
    LintCombine::IdeTraitsFactory ideTraitsFactory;
    auto prepareInputs = ideTraitsFactory.getPrepareInputsInstance( cmdLine );
    const LintCombine::DiagnosticWorker diagnosticWorker( cmdLine, argc == 1 );
    cmdLine = prepareInputs->transformCmdLine( cmdLine );
    prepareInputs->transformFiles();

    if( diagnosticWorker.printDiagnostics( prepareInputs->diagnostics() ) || cmdLine.empty() ) {
        return LintCombine::ExitCode::Success;
    }

    std::unique_ptr< LintCombine::LinterItf > lintCombine;
    try {
        cmdLine.insert( cmdLine.begin(), "LinterCombine" );
        lintCombine = LintCombine::UsualLinterFactory::getInstance().createLinter( cmdLine );
    }
    catch( const LintCombine::Exception & ex ) {
        diagnosticWorker.printDiagnostics( ex.diagnostics() );
        return LintCombine::ExitCode::FailedToConstructLinterCombine;
    }

    lintCombine->callLinter();
    const auto callReturnCode = lintCombine->waitLinter();
    if( callReturnCode == LintCombine::WaitLinterReturnCode::AllLintersFailed ) {
        diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );
        if( ideTraitsFactory.getIdeBehaviorInstance() &&
            !ideTraitsFactory.getIdeBehaviorInstance()->isLinterExitCodeTolerant() ) {
            return LintCombine::ExitCode::FailedToCallLinters;
        }
    }

    if( ideTraitsFactory.getIdeBehaviorInstance() &&
        ideTraitsFactory.getIdeBehaviorInstance()->mayYamlFileContainDocLink() ) {
        const auto callTotals = lintCombine->updateYaml();
        if( !callTotals.successNum ) {
            diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );
            return LintCombine::ExitCode::FailedToUpdateYaml;
        }
    }

    if( lintCombine->getYamlPath().empty() ) {
        diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );
        return LintCombine::ExitCode::FailedToPutDiagsIntoYaml;
    }
    diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );

    return 0;
}
