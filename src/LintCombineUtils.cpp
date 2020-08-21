#include "LintCombineUtils.h"

#include <boost/program_options.hpp>

LintCombine::stringVector LintCombine::moveCmdLineIntoSTLContainer( const int argc, char ** argv ) {
    stringVector cmdLine;
    for( auto i = 1; i < argc; ++i ) {
        cmdLine.emplace_back( std::string( argv[i] ) );
    }
    fixHyphensInCmdLine( cmdLine );
    return cmdLine;
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
