#include "ClangTidyWrapper.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

LintCombine::ClangTidyWrapper::ClangTidyWrapper( int argc, char ** argv, FactoryBase::Services & service )
        : LinterBase( service ) {
    name = "clang-tidy";
    parseCommandLine( argc, argv );
}

void LintCombine::ClangTidyWrapper::updateYamlAction( const YAML::Node & yamlNode ) const {
    addDocLinkToYaml( yamlNode );
}

void LintCombine::ClangTidyWrapper::addDocLinkToYaml( const YAML::Node & yamlNode ) {
    for( auto it : yamlNode[ "Diagnostics" ] ) {
        std::ostringstream documentationLink;
        documentationLink << "https://clang.llvm.org/extra/clang-tidy/checks/" << it[ "DiagnosticName" ] << ".html";
        it[ "Documentation link" ] = documentationLink.str();
    }
}

void LintCombine::ClangTidyWrapper::parseCommandLine( int argc, char ** argv ) {
    po::options_description programOptions;
    programOptions.add_options()
            ( "export-fixes", po::value < std::string >( & yamlPath ),
              "YAML file to store suggested fixes in. The"
              "stored fixes can be applied to the input source"
              "code with clang-apply-replacements." );

    const po::parsed_options parsed
            = po::command_line_parser( argc, argv ).options( programOptions ).allow_unregistered().run();
    po::variables_map vm;
    po::store( parsed, vm );
    notify( vm );
    std::vector < std::string > linterOptionsVec = po::collect_unrecognized( parsed.options, po::include_positional );

    for( const auto & it : linterOptionsVec ) {
        options.append( it + " " );
    }
}
