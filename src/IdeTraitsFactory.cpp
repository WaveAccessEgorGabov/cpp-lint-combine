#include "IdeTraitsFactory.h"
#include "PrepareInputsCLion.h"
#include "PrepareInputsReSharper.h"
#include "PrepareInputsVerbatim.h"

#include <boost/algorithm/string/case_conv.hpp>

LintCombine::IdeTraitsFactory::IdeBehaviorItf *
LintCombine::IdeTraitsFactory::getIdeBehaviorInstance() {
    boost::algorithm::to_lower( ideName );
    if( ideName == "resharper" ) {
        return new IdeBehaviorBase( /*doesAddLinkVal*/true );
    }
    if( ideName == "clion" ) {
        return new IdeBehaviorBase( /*doesAddLinkVal*/false );
    }
    if( ideName.empty() ) {
        return new IdeBehaviorBase( /*doesAddLinkVal*/true );
    }
    return nullptr;
};

LintCombine::PrepareInputsItf *
LintCombine::IdeTraitsFactory::getPrepareCmdLineInstance( stringVector & cmdLine ) {
    if( cmdLine.empty() ) {
        return new PrepareCmdLineOnError( Level::Error, "Command Line is empty",
                                          "FactoryPreparer",  1, 0 );
    }
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
                   [this]( const std::string & str ) -> bool {
                       return str.find( "--ide-profile" ) == 0 || str == ideName;
                   } ), std::end( cmdLine ) );
    if( ideName.empty() ) {
        return new PrepareInputsVerbatim();
    }
    const auto ideNameCopy = ideName;
    boost::algorithm::to_lower( ideName );
    if( ideName == "resharper" ) {
        return new PrepareInputsReSharper();
    }
    if( ideName == "clion" ) {
        return new PrepareInputsCLion();
    }
    // TODO: find position of incorrect IDE in source cmdLine
    return new PrepareCmdLineOnError(
             Level::Error, "\"" + ideNameCopy +
             "\" is not a supported IDE profile",
             "FactoryPreparer",  1, 0 );
}
