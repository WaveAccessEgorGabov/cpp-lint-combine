#include "LinterCombine.h"
#include "LintCombineUtils.h"

#include <iostream>

int main( const int argc, char * argv[] ) {
    // TODO: check --verbatim-commands option
    LintCombine::stringVector commandLine = LintCombine::moveCommandLineToSTLContainer( argc, argv );
    LintCombine::CommandLineOptions commandLinePreparer;
    commandLinePreparer.prepareCommandLineForReSharper ( commandLine );

    for( const auto & it : commandLine ) {
        std::cout << it << " ";
    }
    std::cout << std::endl;
    try {
        LintCombine::LinterCombine linterCombine( commandLine );
        if( linterCombine.printTextIfRequested() ) {
            return 0;
        }
        linterCombine.callLinter();
        const int linterReturnCode = linterCombine.waitLinter();
        if( linterReturnCode == 2 ) {
            std::cerr << "some linters are failed" << std::endl;
        }
        if( linterReturnCode == 3 ) {
            std::cerr << "all linters are failed" << std::endl;
        }
        const LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        if( callTotals.failNum ) {
            std::cout << "Updating " << callTotals.failNum << " yaml-files was failed" << std::endl;
        }
        linterCombine.getYamlPath();
    }
    catch( const std::logic_error & ex ) {
        std::cerr << "std::logic_error exception. What(): " << ex.what() << std::endl;
    }
    catch( const std::exception & ex ) {
        std::cerr << "Exception. What(): " << ex.what() << std::endl;
    }
    return 0;
}
