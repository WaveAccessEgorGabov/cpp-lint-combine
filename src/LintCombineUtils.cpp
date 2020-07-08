#include "LintCombineUtils.h"

#include <boost/program_options.hpp>

LintCombine::stringVector LintCombine::cmdLineToSTLContainer( const int argc, char ** argv ) {
    stringVector commandLine;
    for( auto i = 1; i < argc; ++i ) {
        commandLine.emplace_back( std::string( argv[i] ) );
    }
    return commandLine;
}

void LintCombine::fixHyphensInCmdLine( stringVector & cmdLine ) {
    for( auto & it : cmdLine ) {
        if( it.find( "--" ) != 0 && it.find( '-' ) == 0 ) {
            if( it.find( '=' ) != std::string::npos ) {
                // -param=value -> --param=value
                if( it.find( '=' ) != std::string( "-p" ).size() ) {
                    it.insert( 0, "-" );
                }
            }
            // -param value -> --param value
            else if( it.size() > std::string( "-p" ).size() ) {
                it.insert( 0, "-" );
            }
        }
        if( it.find( "--" ) == 0 ) {
            if( it.find( '=' ) != std::string::npos ) {
                // --p=value -> -p=value
                if( it.find( '=' ) == std::string( "--p" ).size() ) {
                    it.erase( it.begin() );
                }
            }
            // --p value -> -p value
            else if( it.size() == std::string( "--p" ).size() ) {
                it.erase( it.begin() );
            }
        }
    }
}