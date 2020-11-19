#include "IdeTraitsFactory.h"
#include "PrepareInputsCLion.h"
#include "PrepareInputsReSharper.h"
#include "PrepareInputsBareMSVC.h"
#include "PrepareInputsVerbatim.h"
#include "IdeBehaviorBase.h"

#include <boost/algorithm/string/case_conv.hpp>

LintCombine::IdeTraitsFactory::IdeTraitsFactory( StringVector & cmdLine )
    : m_cmdLine( cmdLine ) {}

std::unique_ptr< LintCombine::IdeBehaviorItf >
LintCombine::IdeTraitsFactory::getIdeBehaviorInstance() {
    boost::algorithm::to_lower( m_ideName );
    if( m_ideName == "baremsvc" ) {
        return std::make_unique< IdeBehaviorBase >( /*convertLinterOutput*/ true,
                                                    /*mergeStdoutAndStderr*/true,
                                                    /*mayYamlFileContainDocLink=*/false,
                                                    /*linterExitCodeTolerant=*/false );
    }
    if( m_ideName == "resharper" ) {
        return std::make_unique< IdeBehaviorBase >( /*convertLinterOutput*/ false,
                                                    /*mergeStdoutAndStderr*/false,
                                                    /*mayYamlFileContainDocLink=*/true,
                                                    /*linterExitCodeTolerant=*/false );
    }
    if( m_ideName == "clion" ) {
        if constexpr( BOOST_OS_WINDOWS ) {
            return std::make_unique< IdeBehaviorBase >( /*convertLinterOutput*/ false,
                                                        /*mergeStdoutAndStderr*/false,
                                                        /*mayYamlFileContainDocLink=*/false,
                                                        /*linterExitCodeTolerant=*/false );
        }
        if constexpr( BOOST_OS_LINUX ) {
            return std::make_unique< IdeBehaviorBase >( /*convertLinterOutput*/ false,
                                                        /*mergeStdoutAndStderr*/false,
                                                        /*mayYamlFileContainDocLink=*/false,
                                                        /*linterExitCodeTolerant=*/true );
        }
    }
    if( m_ideName.empty() ) { // verbatim mode
        return std::make_unique< IdeBehaviorBase >( /*convertLinterOutput*/ false,
                                                    /*mergeStdoutAndStderr*/false,
                                                    /*mayYamlFileContainDocLink*/true,
                                                    /*linterExitCodeTolerant*/false );
    }
    return nullptr;
};

std::unique_ptr< LintCombine::PrepareInputsItf >
LintCombine::IdeTraitsFactory::getPrepareInputsInstance() {
    if( m_cmdLine.empty() ) {
        return std::make_unique< PrepareInputsOnError >(
            Level::Error, "Command Line is empty", "FactoryPreparer", 1, 0 );
    }
    boost::program_options::options_description programDesc;
    programDesc.add_options()(
        "ide-profile", boost::program_options::value< std::string >( &m_ideName ) );
    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( m_cmdLine ).
               options( programDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & ex ) {
        return std::make_unique< PrepareInputsOnError >(
            Level::Error, ex.what(), "FactoryPreparer", 1, 0 );
    }
    m_cmdLine.erase( std::remove_if( m_cmdLine.begin(), m_cmdLine.end(),
                   [this]( const std::string & str ) -> bool {
                       return str.find( "--ide-profile" ) == 0 || str == m_ideName;
                   } ), m_cmdLine.end() );
    if( m_ideName.empty() ) {
        return std::make_unique< PrepareInputsVerbatim >();
    }
    const auto ideNameCopy = m_ideName;
    boost::algorithm::to_lower( m_ideName );
    if( m_ideName == "baremsvc" ) {
        return std::make_unique< PrepareInputsBareMSVC >();
    }
    if( m_ideName == "resharper" ) {
        return std::make_unique< PrepareInputsReSharper >();
    }
    if( m_ideName == "clion" ) {
        return std::make_unique< PrepareInputsCLion >();
    }

    return std::make_unique< PrepareInputsOnError >(
        Level::Error, "\"" + ideNameCopy + "\" is not a supported IDE profile", "FactoryPreparer", 1, 0 );
}
