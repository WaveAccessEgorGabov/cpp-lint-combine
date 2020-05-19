#include "LinterCombine.h"

#include <iostream>

int main( const int argc, char * argv[] ) {
    try {
        LintCombine::LinterCombine linterCombine( argc, argv );
        if( linterCombine.printTextIfRequested() ) {
            return 0;
        }
        linterCombine.callLinter();
        const int linterReturnCode = linterCombine.waitLinter();
        if( linterReturnCode == 1 ) {
            std::cerr << "some linters are failed" << std::endl;
        }
        if( linterReturnCode == 3 ) {
            std::cerr << "all linters are failed" << std::endl;
        }
        const LintCombine::CallTotals callTotals = linterCombine.updateYaml();
        if( callTotals.fail ) {
            std::cout << "Updating " << callTotals.fail << " yaml-files was failed" << std::endl;
        }
    }
    catch( const std::logic_error & ex ) {
        std::cerr << ex.what() << std::endl;
    }
    return 0;
}
