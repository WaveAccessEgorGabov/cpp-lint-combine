#include "IdeTraitsFactory.h"
#include "PrepareInputsCLion.h"
#include "PrepareInputsReSharper.h"
#include "PrepareInputsBareMSVC.h"
#include "PrepareInputsVerbatim.h"
#include "IdeBehaviorBase.h"
#include "LintCombineException.h"

#include <boost/algorithm/string/case_conv.hpp>

LintCombine::IdeTraitsFactory::IdeTraitsFactory( StringVector & cmdLineVal ) {
    boost::program_options::options_description programDesc;
    programDesc.add_options()(
        "ide-profile", boost::program_options::value< std::string >( &m_ideName ) );
    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( cmdLineVal ).
               options( programDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & ex ) {
        throw Exception ( { Level::Error, ex.what(), "FactoryPreparer", 1, 0 } );
    }

    // Erase "--ide-profile" option and option's value
    cmdLineVal.erase( std::remove_if( cmdLineVal.begin(), cmdLineVal.end(),
                     [this]( const std::string & str ) -> bool {
                         return str.find( "--ide-profile" ) == 0 || str == m_ideName;
                     } ), cmdLineVal.end() );

    // Check that the parsed IDE's name contains in the list of supported IDE
    boost::algorithm::to_lower( m_ideName );
    auto ideFound = false;
    for( const auto & ideName : { "baremsvc", "resharper", "clion" } ) {
        if( ideName == m_ideName )
            ideFound = true;
    }
    if( !ideFound && !m_ideName.empty() ) {
        m_diagnostics.emplace_back( Level::Error,
                                    "\"" + m_ideName + "\" is not a supported IDE profile",
                                    "FactoryPreparer", 1, 0 );
    }
}

std::unique_ptr< LintCombine::IdeBehaviorItf >
LintCombine::IdeTraitsFactory::getIdeBehaviorInstance() const {
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
}

std::vector<LintCombine::Diagnostic> LintCombine::IdeTraitsFactory::diagnostics() const {
    return m_diagnostics;
}

std::unique_ptr< LintCombine::PrepareInputsItf >
LintCombine::IdeTraitsFactory::getPrepareInputsInstance() const {
    if( m_ideName.empty() ) {
        return std::make_unique< PrepareInputsVerbatim >();
    }
    if( m_ideName == "baremsvc" ) {
        return std::make_unique< PrepareInputsBareMSVC >();
    }
    if( m_ideName == "resharper" ) {
        return std::make_unique< PrepareInputsReSharper >();
    }
    if( m_ideName == "clion" ) {
        return std::make_unique< PrepareInputsCLion >();
    }
    return nullptr;
}
