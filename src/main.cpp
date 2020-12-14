#include "LinterCombine.h" // for LintCombine::RetCode
#include "LintCombineUtils.h"
#include "IdeTraitsFactory.h"
#include "DiagnosticOutputHelper.h"
#include "LintCombineException.h"

namespace LintCombine {
    enum class ExitCode{ Success, FailedToMoveCmdLineIntoSTLContainer,
                         FailedToConstructLinterCombine, FailedToUpdateYaml,
                         FailedToCallLinters, FailedToPutDiagsIntoYaml };
}

int _main( const int argc, _char * argv[] ) {
    LintCombine::StringVector cmdLine;
    LintCombine::DiagnosticOutputHelper diagnosticWorker;
    try {
        cmdLine = LintCombine::moveCmdLineIntoSTLContainer( argc, argv );
    }
    catch( const LintCombine::Exception & ex ) {
        diagnosticWorker.printDiagnostics( ex.diagnostics() );
        return static_cast< int >( LintCombine::ExitCode::FailedToMoveCmdLineIntoSTLContainer );
    }
    LintCombine::IdeTraitsFactory ideTraitsFactory( cmdLine );
    auto prepareInputs = ideTraitsFactory.getPrepareInputsInstance();
    diagnosticWorker.setCmdLine( cmdLine );
    cmdLine = prepareInputs->transformCmdLine( cmdLine );
    prepareInputs->transformFiles();
    if( diagnosticWorker.printDiagnostics( prepareInputs->diagnostics() ) || cmdLine.empty() ) {
        return static_cast< int >( LintCombine::ExitCode::Success );
    }

    std::unique_ptr< LintCombine::LinterItf > lintCombine;
    try {
        cmdLine.insert( cmdLine.begin(), "LinterCombine" );
        lintCombine = LintCombine::UsualLinterFactory::getInstance().createLinter( cmdLine );
    }
    catch( const LintCombine::Exception & ex ) {
        diagnosticWorker.printDiagnostics( ex.diagnostics() );
        return static_cast< int >( LintCombine::ExitCode::FailedToConstructLinterCombine );
    }

    const auto ideBehaviorItf = ideTraitsFactory.getIdeBehaviorInstance();
    lintCombine->callLinter( ideBehaviorItf );
    const auto callReturnCode = lintCombine->waitLinter();
    if( callReturnCode == LintCombine::RetCode::RC_TotalFailure ) {
        if( ideBehaviorItf && !ideBehaviorItf->isLinterExitCodeTolerant() ) {
            diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );
            return static_cast< int >( LintCombine::ExitCode::FailedToCallLinters );
        }
    }

    if( ideBehaviorItf && ideBehaviorItf->mayYamlFileContainDocLink() ) {
        lintCombine->updateYaml();
    }

    if( std::string combinedYamlPath; !lintCombine->getYamlPath( combinedYamlPath ).successNum ) {
        diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );
        return static_cast< int >( LintCombine::ExitCode::FailedToPutDiagsIntoYaml );
    }
    diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );
    return static_cast< int >( LintCombine::ExitCode::Success );
}
