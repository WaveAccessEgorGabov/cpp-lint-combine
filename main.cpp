#include "LinterWrapperUtils.h"
#include "LinterWrapperBase.h"

#include <iostream>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main( int argc, char * argv[] ) {
    LinterWrapperBase * linterWrapper;
    try {
        linterWrapper = parseCommandLine ( argc, argv );
    }
    catch ( const po::error & ex ) {
        std::cerr << "Exception while parse command line; what(): " << ex.what () << std::endl;
        return 1;
    }

    if ( !linterWrapper ) {
        std::cerr << "Expected: linter exist" << std::endl;
        return 1;
    }

    const int linterReturnCode = linterWrapper->callLinter ();
    if ( linterReturnCode ) {
        std::cerr << "Error while running linter" << std::endl;
        return linterReturnCode;
    }

    if ( !linterWrapper->createUpdatedYaml () ) {
        std::cerr << "Error while updating .yaml" << std::endl;
        return 1;
    }

    return 0;
}
