#include "LinterCombine.h"
#include "LintCombineUtils.h"
#include "PrepareCmdLineFactory.h"
#include "DiagnosticWorker.h"

#include <boost/algorithm/string/case_conv.hpp>

// TODO: use enum for IDEs and Linters

int main( int argc, char * argv[] ) {
    LintCombine::stringVector cmdLine = LintCombine::cmdLineToSTLContainer( argc, argv );
    std::string ideName;
    auto * prepareCmdLine = LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( cmdLine, ideName );
    const LintCombine::DiagnosticWorker diagnosticWorker( cmdLine, argc == 1 );
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
    const auto callReturnCode = combine.waitLinter();
    if( callReturnCode == 3 ) {
        diagnosticWorker.printDiagnostics( combine.diagnostics() );
        boost::algorithm::to_lower( ideName );
        if( ideName == "resharper" ) {
            return 1;
        }
    }

    // CLion doesn't work if DocLink is added to yaml-file
    boost::algorithm::to_lower( ideName );
    if( ideName == "resharper" ) {
        const auto callTotals = combine.updateYaml();
        if (callTotals.failNum == combine.numLinters()) {
            diagnosticWorker.printDiagnostics(combine.diagnostics());
            return 1;
        }
    }

    if( combine.getYamlPath().empty() ) {
        diagnosticWorker.printDiagnostics( combine.diagnostics() );
        return 1;
    }
    diagnosticWorker.printDiagnostics( combine.diagnostics() );

    return callReturnCode;
}
