#include "LintCombineUtils.h"
#include <fstream>

LintCombine::StringVector LintCombine::moveCmdLineIntoSTLContainer( const int argc, char ** argv ) {
    StringVector cmdLine;
    for( auto i = 1; i < argc; ++i ) {
        cmdLine.emplace_back( argv[i] );
    }
    normalizeHyphensInCmdLine( cmdLine );
    return cmdLine;
}

void LintCombine::normalizeHyphensInCmdLine( StringVector & cmdLine ) {
    for( auto & cmdLineElem : cmdLine ) {
        if( cmdLineElem.find( "--" ) != 0 && cmdLineElem.find( '-' ) == 0 ) {
            if( cmdLineElem.find( '=' ) != std::string::npos ) {
                // -param=value -> --param=value
                if( cmdLineElem.find( '=' ) != 2 ) {
                    cmdLineElem.insert( 0, "-" );
                }
            }
            // -param value -> --param value
            else if( cmdLineElem.size() > 2 ) {
                cmdLineElem.insert( 0, "-" );
            }
        }
        if( cmdLineElem.find( "--" ) == 0 ) {
            if( cmdLineElem.find( '=' ) != std::string::npos ) {
                // --p=value -> -p=value
                if( cmdLineElem.find( '=' ) == 3 ) {
                    cmdLineElem.erase( cmdLineElem.begin() );
                }
            }
            // --p value -> -p value
            else if( cmdLineElem.size() == 3 ) {
                cmdLineElem.erase( cmdLineElem.begin() );
            }
        }
    }
}

bool LintCombine::isFileCreatable( const std::filesystem::path & filePath ) {
    if( exists( filePath ) )
        return true;
    std::error_code errorCode;
    create_directory( filePath.parent_path(), errorCode );
    if( errorCode.value() )
        return false;
    std::ofstream file( filePath );
    if( file.fail() )
        return false;
    file.close();
    std::filesystem::remove( filePath );
    return true;
}
