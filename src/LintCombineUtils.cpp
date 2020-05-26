#include "LintCombineUtils.h"

#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;

char ** prepareCommandLine( int & argc, char ** argv ) {
    po::options_description programOptions;
    std::string pathToResultYaml;
    std::string pathToDiagnosticsDir;
    programOptions.add_options()
            ( "export-fixes", po::value < std::string >( & pathToResultYaml ) )
            ( "p", po::value < std::string >( & pathToDiagnosticsDir ) );
    const po::parsed_options parsed =
            po::command_line_parser( argc, argv ).options( programOptions )
                    .style( po::command_line_style::long_allow_adjacent | po::command_line_style::allow_long_disguise )
                    .allow_unregistered().run();
    po::variables_map variablesMap;
    po::store( parsed, variablesMap );
    notify( variablesMap );
    // MayBe copy vector to deque, for fast push_front
    static std::vector < std::string > linterOptionsVec
            = po::collect_unrecognized( parsed.options, po::include_positional );

    linterOptionsVec.insert( linterOptionsVec.begin(), "--sub-linter=clang-tidy" );
    linterOptionsVec.insert( linterOptionsVec.begin(), pathToResultYaml.insert( 0, "-export-fixes=" ) );
    linterOptionsVec.insert( linterOptionsVec.begin(), "" );
    linterOptionsVec.push_back( "-p=" + pathToDiagnosticsDir );
    linterOptionsVec.push_back( "-export-fixes=" + pathToDiagnosticsDir + "/diagnosticsClangTidy.yaml" );
    linterOptionsVec.emplace_back( "--sub-linter=clazy-standalone" );
    linterOptionsVec.push_back( "-p=" + pathToDiagnosticsDir );
    linterOptionsVec.push_back( "-export-fixes=" + pathToDiagnosticsDir + "/diagnosticsClazy.yaml" );
    linterOptionsVec.emplace_back( "-checks=level1" );
    argc = linterOptionsVec.size();

    char ** result = new char * [linterOptionsVec.size()];
    for( size_t i = 0; i < linterOptionsVec.size(); ++i ) {
        result[ i ] = const_cast< char * > ( linterOptionsVec[ i ].c_str() );
    }
    return result;
}
