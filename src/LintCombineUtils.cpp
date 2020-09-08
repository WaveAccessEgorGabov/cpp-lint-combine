#include "LintCombineUtils.h"
#include <fstream>

LintCombine::StringVector LintCombine::moveCmdLineIntoSTLContainer( const int argc, char ** argv ) {
    StringVector cmdLine;
    for( auto i = 1; i < argc; ++i ) {
        cmdLine.emplace_back( argv[i] );
    }
    fixHyphensInCmdLine( cmdLine );
    return cmdLine;
}

void LintCombine::fixHyphensInCmdLine( StringVector & cmdLine ) {
    enum StrSize{ TwoSymbol = 2, ThreeSymbols = 3 };
    for( auto & cmdLineElement : cmdLine ) {
        if( cmdLineElement.find( "--" ) != 0 && cmdLineElement.find( '-' ) == 0 ) {
            if( cmdLineElement.find( '=' ) != std::string::npos ) {
                // -param=value -> --param=value
                if( cmdLineElement.find( '=' ) != StrSize::TwoSymbol ) {
                    cmdLineElement.insert( 0, "-" );
                }
            }
            // -param value -> --param value
            else if( cmdLineElement.size() > StrSize::TwoSymbol ) {
                cmdLineElement.insert( 0, "-" );
            }
        }
        if( cmdLineElement.find( "--" ) == 0 ) {
            if( cmdLineElement.find( '=' ) != std::string::npos ) {
                // --p=value -> -p=value
                if( cmdLineElement.find( '=' ) == StrSize::ThreeSymbols ) {
                    cmdLineElement.erase( cmdLineElement.begin() );
                }
            }
            // --p value -> -p value
            else if( cmdLineElement.size() == StrSize::ThreeSymbols ) {
                cmdLineElement.erase( cmdLineElement.begin() );
            }
        }
    }
}

bool LintCombine::isFileCreatable( const std::filesystem::path & filePath ) {
    std::error_code errorCode;
    create_directory( filePath.parent_path(), errorCode );
    if( errorCode.value() ) { return false; }
    std::ofstream file( filePath );
    if( file.fail() ) { return false; }
    file.close();
    std::filesystem::remove( filePath );
    return true;
}
