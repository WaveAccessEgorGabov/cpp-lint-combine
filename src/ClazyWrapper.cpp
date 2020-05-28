#include "ClazyWrapper.h"

#include <iostream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;

LintCombine::ClazyWrapper::ClazyWrapper( stringVectorConstRef commandLineSTL,
                                         FactoryBase::Services & service )
        : LinterBase( service ) {
}

LintCombine::ClazyWrapper::ClazyWrapper( int argc, char ** argv, FactoryBase::Services & service )
        : LinterBase( service ) {
    name = "clazy-standalone";
    parseCommandLine( argc, argv );
}

void LintCombine::ClazyWrapper::updateYamlAction( const YAML::Node & yamlNode ) const {
    addDocLinkToYaml( yamlNode );
}

void LintCombine::ClazyWrapper::parseCommandLine( stringVectorConstRef commandLineSTL ) {
}

void LintCombine::ClazyWrapper::parseCommandLine( int argc, char ** argv ) {
    po::options_description programOptions;
    programOptions.add_options()
            ( "export-fixes", po::value < std::string >( & yamlPath ) );

    const po::parsed_options parsed =
            po::command_line_parser( argc, argv ).options( programOptions ).allow_unregistered().run();
    po::variables_map variablesMap;
    po::store( parsed, variablesMap );
    notify( variablesMap );
    std::vector < std::string > linterOptionsVec = po::collect_unrecognized( parsed.options, po::include_positional );

    for( const auto & it : linterOptionsVec ) {
        options.append( it + " " );
    }

    std::string yamlFileName = boost::filesystem::path( yamlPath ).filename().string();
    if( !boost::filesystem::portable_name( yamlFileName ) || !boost::filesystem::exists( yamlPath ) ) {
        std::cerr << yamlFileName << " not exist or invalid!" << std::endl;
        yamlPath = std::string();
    }
}

void LintCombine::ClazyWrapper::addDocLinkToYaml( const YAML::Node & yamlNode ) {
    for( auto it : yamlNode[ "Diagnostics" ] ) {
        std::ostringstream documentationLink;
        std::ostringstream diagnosticName;
        diagnosticName << it[ "DiagnosticName" ];
        documentationLink << "https://github.com/KDE/clazy/blob/master/docs/checks/README-"
                          << diagnosticName.str().substr( std::string( "clazy-" ).size(), diagnosticName.str().size() )
                          << ".md";
        it[ "Documentation link" ] = documentationLink.str();
    }
}
