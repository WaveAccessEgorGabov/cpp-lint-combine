#include "LinterWrapperUtils.h"
#include "LinterWrapperBase.h"

#include <iostream>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main( const int argc, char * argv[] ) {
    LinterWrapperBase * linterWrapper = nullptr;
    bool isNeedHelp;
    try {
        linterWrapper = parseCommandLine ( argc, argv, isNeedHelp );
    }
    catch ( const po::error & ex ) {
        std::cerr << "Exception while parse command line; what(): " << ex.what () << std::endl;
        return 1;
    }

    if ( !linterWrapper ) {
		if( isNeedHelp ) {
			return 0;
		}
        std::cerr << "Expected: linter exist" << std::endl;
        return 1;
    }

    const int linterReturnCode = linterWrapper->callLinter ( isNeedHelp );
    if ( linterReturnCode ) {
        std::cerr << "Error while running linter" << std::endl;
        return linterReturnCode;
    }

    if ( !isNeedHelp ) {
        if ( !linterWrapper->createUpdatedYaml () ) {
            std::cerr << "Error while updating .yaml" << std::endl;
            return 1;
        }
    }

    return 0;
}
