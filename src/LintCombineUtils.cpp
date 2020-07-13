#include "LintCombineUtils.h"

#include <iostream>

#include <boost/algorithm/string/join.hpp>
#include <boost/program_options.hpp>

LintCombine::stringVector LintCombine::cmdLineToSTLContainer( const int argc, char ** argv ) {
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

LintCombine::stringVector
LintCombine::prepareDiagnostics( const stringVector & cmdLine,
                                 const std::vector< Diagnostic > & diagnostics ) {
    const auto sourceCL = boost::algorithm::join( cmdLine, " " );
    stringVector preparedOutput;
    for( const auto & it : diagnostics ) {
        std::string levelAsString;
        switch( it.level ) {
            case Level::Trace:   { levelAsString = "Trace";   break; }
            case Level::Debug:   { levelAsString = "Debug";   break; }
            case Level::Info:    { levelAsString = "Info";    break; }
            case Level::Warning: { levelAsString = "Warning"; break; }
            case Level::Error:   { levelAsString = "Error";   break; }
            case Level::Fatal:   { levelAsString = "Fatal";   break; }
        }
        preparedOutput.emplace_back( levelAsString + ": " + it.text );
        if( it.firstPos < it.lastPos ) {
            std::string errorShow = sourceCL + "\n";
            errorShow.append( it.firstPos, ' ' );
            errorShow.append( it.lastPos - it.firstPos, '~' );
            preparedOutput.emplace_back( errorShow );
        }
    }
    return preparedOutput;
}

void LintCombine::printDiagnostics( const stringVector & cmdLine,
                                    const std::vector< Diagnostic > & diagnostics ) {
    for( const auto & it : prepareDiagnostics( cmdLine, diagnostics ) ) {
        std::cerr << it << std::endl;
    }
}
