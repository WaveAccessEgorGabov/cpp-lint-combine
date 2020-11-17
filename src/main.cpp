#include "LinterCombine.h" // for LintCombine::RetCode
#include "LintCombineUtils.h"
#include "IdeTraitsFactory.h"
#include "DiagnosticOutputHelper.h"

namespace LintCombine {
    enum ExitCode{ Success, FailedToConstructLinterCombine, FailedToUpdateYaml,
                   FailedToCallLinters, FailedToPutDiagsIntoYaml };
}

int main( int argc, char * argv[] ) {
    LintCombine::StringVector cmdLine = LintCombine::moveCmdLineIntoSTLContainer( argc, argv );
    LintCombine::IdeTraitsFactory ideTraitsFactory( cmdLine );
    auto prepareInputs = ideTraitsFactory.getPrepareInputsInstance();
    const LintCombine::DiagnosticOutputHelper diagnosticWorker( cmdLine );
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
    if( callReturnCode == LintCombine::RetCode::RC_TotalFailure ) {
        if( ideTraitsFactory.getIdeBehaviorInstance() &&
            !ideTraitsFactory.getIdeBehaviorInstance()->isLinterExitCodeTolerant() ) {
            diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );
            return LintCombine::ExitCode::FailedToCallLinters;
        }
    }

    if( ideTraitsFactory.getIdeBehaviorInstance() &&
        ideTraitsFactory.getIdeBehaviorInstance()->mayYamlFileContainDocLink() ) {
        lintCombine->updateYaml();
    }

    if( std::string combinedYamlPath; !lintCombine->getYamlPath( combinedYamlPath ).successNum ) {
        diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );
        return LintCombine::ExitCode::FailedToPutDiagsIntoYaml;
    }
    diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );
    return LintCombine::ExitCode::Success;
}
