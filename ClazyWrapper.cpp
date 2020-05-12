#include "ClazyWrapper.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

LintCombine::ClazyWrapper::ClazyWrapper( int argc, char ** argv, FactoryBase::Services & service )
        : LinterBase( service ) {
    name = "clazy-standalone";
    parseCommandLine( argc, argv );
}

void LintCombine::ClazyWrapper::updateYamlAction( const YAML::Node & yamlNode ) const {
    addDocLinkToYaml( yamlNode );
}

void LintCombine::ClazyWrapper::addDocLinkToYaml( const YAML::Node & yamlNode ) const {
    for( auto it : yamlNode[ "Diagnostics" ] ) {
        std::ostringstream documentationLink;
        std::ostringstream diagnosticName;
        diagnosticName << it[ "DiagnosticName" ];
        documentationLink << "https://github.com/KDE/clazy/blob/master/docs/checks/README-";
        // substr() from 6 to size() for skipping "clazy-" in DiagnosticName
        documentationLink << diagnosticName.str().substr( 6, diagnosticName.str().size() ) << ".md";
        it[ "Documentation link" ] = documentationLink.str();
    }
}

void LintCombine::ClazyWrapper::parseCommandLine( int argc, char ** argv ) {
    po::options_description programOptions;
    programOptions.add_options()
            ( "help,h", "Display available options" )
            ( "export-fixes", po::value < std::string >( & yamlPath ),
              "YAML file to store suggested fixes in. The"
              "stored fixes can be applied to the input source"
              "code with clang-apply-replacements." );

    po::parsed_options parsed =
            po::command_line_parser( argc, argv ).options( programOptions ).allow_unregistered().run();
    po::variables_map vm;
    po::store( parsed, vm );
    notify( vm );
    std::vector < std::string > linterOptionsVec = po::collect_unrecognized( parsed.options, po::include_positional );

    for( const auto & it : linterOptionsVec ) {
        options.append( it + " " );
    }
}

