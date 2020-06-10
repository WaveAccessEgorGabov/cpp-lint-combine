#include "LintCombineUtils.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iostream>

void LintCombine::CommandLineOptions::addOptionToClangTidy( const std::string & option ) {
    lintersOptions[0]->options.emplace_back(option);
}

void LintCombine::CommandLineOptions::addOptionToAllLinters( const std::string & option ) {
    for( const auto & it : lintersOptions ) {
        it->options.emplace_back( option );
    }
}

// TODO: use std::copy and back_inserter
void LintCombine::CommandLineOptions::appendLintersOptionToCommandLine( stringVector & commandLine ) const {
    for ( const auto & i : lintersOptions ) {
        for(const auto & j : i->options) {
            commandLine.emplace_back( j );
        }
    }
}

void LintCombine::CommandLineOptions::initLintCombineOptions( stringVector  & commandLine ) const {
    commandLine.emplace_back("--result-yaml=" + pathToCommonYaml);
}

void LintCombine::CommandLineOptions::initUnrecognizedOptions() {
    for (auto & unrecognized : unrecognizedCollection) {
        boost::algorithm::replace_all(unrecognized, "\"", "\\\"");
        std::string strToCompare = "-config=";
        if ( unrecognized.find( strToCompare ) == 0 ) {
            unrecognized = optionValueToQuotes(strToCompare, unrecognized);
            addOptionToClangTidy(unrecognized);
            continue;
        }
        strToCompare = "-line-filter=";
        if (unrecognized.find(strToCompare) == 0) {
            unrecognized = optionValueToQuotes(strToCompare, unrecognized);
            addOptionToClangTidy(unrecognized);
            continue;
        }
        strToCompare = "-header-filter=";
        if (unrecognized.find(strToCompare) == 0) {
            addOptionToAllLinters(unrecognized);
            continue;
        }
        // File to analize
        if (unrecognized[0] != '@' && unrecognized[0] != '-') {
            addOptionToAllLinters(unrecognized);
            continue;
        }
        addOptionToClangTidy(unrecognized);
    }
}

void LintCombine::CommandLineOptions::initCommandLine( stringVector & commandLine ) {
    commandLine.clear();
    lintersOptions = { new ClangTidyOptions( pathToWorkDir ) ,
                       new ClazyOptions( pathToWorkDir, clazyChecks ) };
    initLintCombineOptions ( commandLine );
    initUnrecognizedOptions();
    appendLintersOptionToCommandLine( commandLine );
}

std::string LintCombine::CommandLineOptions::optionValueToQuotes(const std::string & optionName,
                                       const std::string & optionNameWithValue) {
    return optionName +"\"" +
        optionNameWithValue.substr(optionName.size(), std::string::npos) + "\"";
}

// TODO: Figure out with one and two hyphens
void LintCombine::CommandLineOptions::prepareCommandLineForReSharper( stringVector & commandLine ) {
    boost::program_options::options_description programOptions;
    programOptions.add_options()
        ( "verbatim-commands", "pass options verbatim" )
        ( "clazy-checks", boost::program_options::value < std::string > ( &clazyChecks ) )
        ( "export-fixes", boost::program_options::value < std::string >( &pathToCommonYaml ) )
        ( "p", boost::program_options::value < std::string >( &pathToWorkDir ) );
    const boost::program_options::parsed_options parsed =
        boost::program_options::command_line_parser( commandLine ).options( programOptions )
        .style( boost::program_options::command_line_style::long_allow_adjacent |
                boost::program_options::command_line_style::allow_long_disguise )
        .allow_unregistered().run();
    boost::program_options::variables_map variablesMap;
    store(parsed, variablesMap);
    notify(variablesMap);
    if( variablesMap.count( "verbatim-commands") ) {
        return;
    }
    unrecognizedCollection =
        collect_unrecognized(parsed.options, boost::program_options::include_positional);
    initCommandLine( commandLine );
}

LintCombine::stringVector LintCombine::moveCommandLineToSTLContainer( const int argc, char ** argv ) {
    stringVector commandLine;
    for( auto i = 1; i < argc; ++i ) {
        commandLine.emplace_back( std::string( argv[i] ) );
    }
    return commandLine;
}
