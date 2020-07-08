#include "PrepareCmdLineFactory.h"

LintCombine::PrepareCmdLineItf *
LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( stringVector & cmdLine ) {
    if( cmdLine.empty() ) {
        return new PrepareCmdLineOnError( "Command line is empty",
                                     Level::Error, 1, 0 );
    }
    fixHyphensInCmdLine( cmdLine );
    std::string ideName;
    boost::program_options::options_description programDesc;
    programDesc.add_options()
        ( "ide-profile",
          boost::program_options::value < std::string >( &ideName ) );
    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( cmdLine ).
               options( programDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & ex ) {
        return new PrepareCmdLineOnError( ex.what(), Level::Error, 1, 0 );
    }
    cmdLine.erase( std::remove_if( std::begin( cmdLine ), std::end( cmdLine ),
                   [ideName]( const std::string & str ) -> bool {
                       return str.find( "--ide-profile" ) == 0 || str == ideName;
                   } ), std::end( cmdLine ) );
    if( ideName.empty() ) {
        return new PrepareCmdLineOnError( "Options were passed verbatim",
                                          Level::Info, 1, 0 );
    }
    const auto ideNameCopy = ideName;
    boost::algorithm::to_lower( ideName );
    if( ideName == "resharper" ) {
        return new PrepareCmdLineReSharper();
    }
    // TODO: find position of incorrect IDE
    return new PrepareCmdLineOnError( "\"" + ideNameCopy +
                                      "\" isn't supported by cpp-lint-combine",
                                      Level::Error, 1, 0 );
}

void LintCombine::PrepareCmdLineFactory::fixHyphensInCmdLine( stringVector & cmdLine ) {
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