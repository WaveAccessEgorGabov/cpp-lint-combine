#include "IdeTraitsFactory.h"
#include "PrepareInputsCLion.h"
#include "PrepareInputsReSharper.h"
#include "PrepareInputsVerbatim.h"

#include <boost/algorithm/string/case_conv.hpp>

std::shared_ptr< LintCombine::IdeTraitsFactory::IdeBehaviorItf >
LintCombine::IdeTraitsFactory::getIdeBehaviorInstance() {
    boost::algorithm::to_lower( ideName );
    if( ideName == "resharper" ) {
        return std::make_shared< IdeBehaviorBase >( /*m_yamlContainsDocLink*/true );
    }
    if( ideName == "clion" ) {
        return std::make_shared< IdeBehaviorBase >( /*m_yamlContainsDocLink*/false );
    }
    if( ideName.empty() ) {
        return std::make_shared< IdeBehaviorBase >( /*m_yamlContainsDocLink*/true );
    }
    return nullptr;
};

std::shared_ptr< LintCombine::PrepareInputsItf >
LintCombine::IdeTraitsFactory::getPrepareCmdLineInstance( stringVector & cmdLine ) {
    if( cmdLine.empty() ) {
        return std::make_shared< PrepareCmdLineOnError >(
            Level::Error, "Command Line is empty", "FactoryPreparer", 1, 0 );
    }
    boost::program_options::options_description programDesc;
    programDesc.add_options()
        ( "ide-profile",
          boost::program_options::value< std::string >( &ideName ) );
    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( cmdLine ).
               options( programDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & ex ) {
        return std::make_shared< PrepareCmdLineOnError >(
            Level::Error, ex.what(), "FactoryPreparer", 1, 0 );
    }
    cmdLine.erase( std::remove_if( std::begin( cmdLine ), std::end( cmdLine ),
                   [this]( const std::string & str ) -> bool {
        return str.find( "--ide-profile" ) == 0 || str == ideName;
    } ), std::end( cmdLine ) );
    if( ideName.empty() ) {
        return std::make_shared< PrepareInputsVerbatim >();
    }
    const auto ideNameCopy = ideName;
    boost::algorithm::to_lower( ideName );
    if( ideName == "resharper" ) {
        return std::make_shared< PrepareInputsReSharper >();
    }
    if( ideName == "clion" ) {
        return std::make_shared< PrepareInputsCLion >();
    }

    return std::make_shared< PrepareCmdLineOnError >(
        Level::Error, "\"" + ideNameCopy +
        "\" is not a supported IDE profile",
        "FactoryPreparer", 1, 0 );
}
