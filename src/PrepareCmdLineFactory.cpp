#include "PrepareCmdLineFactory.h"
#include "PrepareCmdLineReSharper.h"
#include "PrepareCmdLineVerbatim.h"

#include <boost/algorithm/string/case_conv.hpp>

LintCombine::PrepareCmdLineItf *
LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( stringVector & cmdLine ) {
    if( cmdLine.empty() ) {
        return new PrepareCmdLineOnError( "Command line is empty",
                                          "PrepareCmdLineFactory", Level::Error, 1, 0 );
    }
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
        return new PrepareCmdLineOnError( ex.what(), "PrepareCmdLineFactory", Level::Error, 1, 0 );
    }
    cmdLine.erase( std::remove_if( std::begin( cmdLine ), std::end( cmdLine ),
                   [ideName]( const std::string & str ) -> bool {
                       return str.find( "--ide-profile" ) == 0 || str == ideName;
                   } ), std::end( cmdLine ) );
    if( ideName.empty() ) {
        return new PrepareCmdLineVerbatim();
    }
    const auto ideNameCopy = ideName;
    boost::algorithm::to_lower( ideName );
    if( ideName == "resharper" ) {
        return new PrepareCmdLineReSharper();
    }
    // TODO: find position of incorrect IDE in source cmdLine
    return new PrepareCmdLineOnError( "\"" + ideNameCopy +
                                      "\" isn't supported by cpp-lint-combine",
                                      "PrepareCmdLineFactory", Level::Error, 1, 0 );
}
