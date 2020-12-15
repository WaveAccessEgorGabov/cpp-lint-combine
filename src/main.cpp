#include "LinterCombine.h" // for LintCombine::RetCode
#include "LintCombineUtils.h"
#include "IdeTraitsFactory.h"
#include "OutputHelper.h"
#include "LintCombineException.h"

namespace LintCombine {
    enum class ExitCode{ Success, FailedToParseIdeName,
                         FailedToMoveCmdLineIntoSTLContainer,
                         FailedToConstructLinterCombine, FailedToUpdateYaml,
                         FailedToCallLinters, FailedToPutDiagsIntoYaml };
}

int _main( const int argc, _char * argv[] ) {
    LintCombine::OutputHelper diagnosticWorker;
    LintCombine::StringVector cmdLine;
    try {
        cmdLine = LintCombine::moveCmdLineIntoSTLContainer( argc, argv );
    }
    catch( const LintCombine::Exception & ex ) {
        diagnosticWorker.printDiagnostics( ex.diagnostics() );
        return static_cast< int >( LintCombine::ExitCode::FailedToMoveCmdLineIntoSTLContainer );
    }

    if( LintCombine::checkCmdLineForEmptiness( cmdLine ) ||
        LintCombine::checkCmdLineForHelpOption( cmdLine ) )
    {
        return static_cast< int >( LintCombine::ExitCode::Success );
    }

    std::unique_ptr< const LintCombine::IdeTraitsFactory > ideTraitsFactory;
    try {
        ideTraitsFactory = std::make_unique< LintCombine::IdeTraitsFactory >( cmdLine );
        diagnosticWorker.setCmdLine( cmdLine );
    }
    catch( const LintCombine::Exception & ex ) {
        diagnosticWorker.printDiagnostics( ex.diagnostics() );
        return static_cast< int >( LintCombine::ExitCode::FailedToParseIdeName );
    }

    auto prepareInputs = ideTraitsFactory->getPrepareInputsInstance();
    if( !prepareInputs ) {
        diagnosticWorker.printDiagnostics( ideTraitsFactory->diagnostics() );
        return static_cast< int >( LintCombine::ExitCode::FailedToParseIdeName );
    }

    const auto ideBehaviorItf = ideTraitsFactory->getIdeBehaviorInstance();
    if( !ideBehaviorItf ) {
        diagnosticWorker.printDiagnostics( ideTraitsFactory->diagnostics() );
        return static_cast< int >( LintCombine::ExitCode::FailedToParseIdeName );
    }

    cmdLine = prepareInputs->transformCmdLine( cmdLine );
    if( cmdLine.empty() ) {
        diagnosticWorker.printDiagnostics( prepareInputs->diagnostics() );
        return static_cast< int >( LintCombine::ExitCode::Success );
    }
    prepareInputs->transformFiles();

    std::unique_ptr< LintCombine::LinterItf > lintCombine;
    try {
        cmdLine.insert( cmdLine.begin(), "LinterCombine" );
        lintCombine = LintCombine::UsualLinterFactory::getInstance().createLinter( cmdLine );
    }
    catch( const LintCombine::Exception & ex ) {
        diagnosticWorker.printDiagnostics( ex.diagnostics() );
        return static_cast< int >( LintCombine::ExitCode::FailedToConstructLinterCombine );
    }

    lintCombine->callLinter( ideBehaviorItf );
    const auto callReturnCode = lintCombine->waitLinter();
    if( callReturnCode == LintCombine::RetCode::RC_TotalFailure &&
        !ideBehaviorItf->isLinterExitCodeTolerant() )
    {
        diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );
        return static_cast< int >( LintCombine::ExitCode::FailedToCallLinters );
    }

    if( ideBehaviorItf->mayYamlFileContainDocLink() ) {
        lintCombine->updateYaml();
    }

    if( std::string combinedYamlPath; !lintCombine->getYamlPath( combinedYamlPath ).successNum ) {
        diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );
        return static_cast< int >( LintCombine::ExitCode::FailedToPutDiagsIntoYaml );
    }
    diagnosticWorker.printDiagnostics( lintCombine->diagnostics() );
    return static_cast< int >( LintCombine::ExitCode::Success );
}
