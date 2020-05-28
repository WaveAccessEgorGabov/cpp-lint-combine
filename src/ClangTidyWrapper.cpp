#include "ClangTidyWrapper.h"

#include <iostream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

LintCombine::ClangTidyWrapper::ClangTidyWrapper( stringVectorConstRef commandLineSTL,
                                                 FactoryBase::Services & service )
        : LinterBase( service ) {
}

LintCombine::ClangTidyWrapper::ClangTidyWrapper( int argc, char ** argv, FactoryBase::Services & service )
        : LinterBase( service ) {
    name = "clang-tidy";
    parseCommandLine( argc, argv );
}

void LintCombine::ClangTidyWrapper::updateYamlAction( const YAML::Node & yamlNode ) const {
    addDocLinkToYaml( yamlNode );
}

void LintCombine::ClangTidyWrapper::parseCommandLine( stringVectorConstRef commandLineSTL ) {
}

void LintCombine::ClangTidyWrapper::parseCommandLine( int argc, char ** argv ) {
    boost::program_options::options_description programOptions;
    programOptions.add_options()
            ( "export-fixes", boost::program_options::value < std::string >( & yamlPath ) );

    const boost::program_options::parsed_options parsed =
            boost::program_options::command_line_parser( argc, argv ).options(
                    programOptions ).allow_unregistered().run();
    boost::program_options::variables_map variablesMap;
    boost::program_options::store( parsed, variablesMap );
    notify( variablesMap );
    std::vector < std::string > linterOptionsVec
            = boost::program_options::collect_unrecognized( parsed.options,
                                                            boost::program_options::include_positional );

    for( const auto & it : linterOptionsVec ) {
        options.append( it + " " );
    }

    std::string yamlFileName = boost::filesystem::path( yamlPath ).filename().string();
    if( !boost::filesystem::portable_name( yamlFileName ) ) {
        std::cerr << "\"" << yamlFileName << "\"" << " is incorrect file name!" << std::endl;
        yamlPath = std::string();
    }
}

void LintCombine::ClangTidyWrapper::addDocLinkToYaml( const YAML::Node & yamlNode ) {
    for( auto it : yamlNode[ "Diagnostics" ] ) {
        std::ostringstream documentationLink;
        documentationLink << "https://clang.llvm.org/extra/clang-tidy/checks/" << it[ "DiagnosticName" ] << ".html";
        it[ "Documentation link" ] = documentationLink.str();
    }
}
