#include "LinterCombine.h"
#include "LintCombineUtils.h"
#include "IdeTraitsFactory.h"
#include "DiagnosticWorker.h"

#include <boost/algorithm/string/case_conv.hpp>

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

    LintCombine::LinterCombine combine( cmdLine );
    if( combine.isErrorOccur() ) {
        diagnosticWorker.printDiagnostics( combine.diagnostics() );
        return 1;
    }

    combine.callLinter();
    const auto callReturnCode = combine.waitLinter();
    if( callReturnCode == 3 ) {
        diagnosticWorker.printDiagnostics( combine.diagnostics() );
        return callReturnCode;
    }

    if( ideTraitsFactory.getIdeBehaviorInstance() &&
        ideTraitsFactory.getIdeBehaviorInstance()->isYamlContainDocLink() ) {
        const auto callTotals = combine.updateYaml();
        if( callTotals.failNum == combine.numLinters() ) {
            diagnosticWorker.printDiagnostics( combine.diagnostics() );
            return 1;
        }
    }

    if( combine.getYamlPath().empty() ) {
        diagnosticWorker.printDiagnostics( combine.diagnostics() );
        return 1;
    }
    diagnosticWorker.printDiagnostics( combine.diagnostics() );

    return 0;
}
