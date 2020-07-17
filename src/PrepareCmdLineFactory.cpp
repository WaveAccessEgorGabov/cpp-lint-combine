#include "PrepareCmdLineFactory.h"
#include "PrepareCmdLineReSharper.h"
#include "PrepareCmdLineCLion.h"
#include "PrepareCmdLineVerbatim.h"

#include <boost/algorithm/string/case_conv.hpp>

LintCombine::PrepareCmdLineItf *
LintCombine::PrepareCmdLineFactory::createInstancePrepareCmdLine( stringVector & cmdLine ) {
    if( cmdLine.empty() ) {
        return new PrepareCmdLineOnError( Level::Error, "Command Line is empty",
                                          "FactoryPreparer",  1, 0 );
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
        return new PrepareCmdLineOnError( Level::Error, ex.what(),
                                          "FactoryPreparer",  1, 0 );
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
    if( ideName == "clion" ) {
        return new PrepareCmdLineCLion();
    }
    // TODO: find position of incorrect IDE in source cmdLine
    return new PrepareCmdLineOnError(
             Level::Error, "\"" + ideNameCopy +
             "\" is not a supported IDE profile",
             "FactoryPreparer",  1, 0 );
}
