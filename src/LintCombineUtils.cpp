#include "LintCombineUtils.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string/replace.hpp>

void LintCombine::CommandLineOptions::addOptionToClangTidy( const std::string & option ) {
    lintersOptions[0]->options.emplace_back(option);
}

void LintCombine::CommandLineOptions::addOptionToAllLinters( const std::string& option ) {
    for( const auto & it : lintersOptions ) {
        it->options.emplace_back( option );
    }
}

void LintCombine::CommandLineOptions::appendLintersOptionToCommandLine( std::vector < std::string > & commandLine ) {
    for ( const auto & i : lintersOptions ) {
        for(const auto & j : i->options) {
            commandLine.emplace_back( j );
        }
    }
}

void LintCombine::CommandLineOptions::initLintCombineOptions( std::vector < std::string >& commandLine ) const {
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
        if (unrecognized[0] != '@' && unrecognized[0] != '-') {
            addOptionToAllLinters(unrecognized);
            continue;
        }
        addOptionToClangTidy(unrecognized);
    }
}

void LintCombine::CommandLineOptions::initCommandLine( std::vector < std::string > & commandLine ) {
    commandLine.clear();
    lintersOptions = { new ClangTidyOptions( pathToWorkDir ) , new ClazyOptions( pathToWorkDir ) };
    initUnrecognizedOptions();
    initLintCombineOptions(commandLine);
    appendLintersOptionToCommandLine( commandLine );
}

std::string LintCombine::CommandLineOptions::optionValueToQuotes(const std::string & optionName,
                                       const std::string & optionNameWithValue) {
    return optionName +"\"" +
        optionNameWithValue.substr(optionName.size(), std::string::npos) + "\"";
}

void LintCombine::prepareCommandLineForReSharper( stringVector & commandLine ) {
    CommandLineOptions commandLineOptions;
    boost::program_options::options_description programOptions;
    std::string pathToResultYaml;
    std::string pathToDiagnosticsDir;
    programOptions.add_options()
        ("export-fixes",
            boost::program_options::value < std::string >( &commandLineOptions.getPathToCommonYamlRef() ) )
        ( "p",
            boost::program_options::value < std::string >( &commandLineOptions.getPathToWorkDirRef() ) );
    const boost::program_options::parsed_options parsed =
        boost::program_options::command_line_parser( commandLine ).options( programOptions )
        .style( boost::program_options::command_line_style::long_allow_adjacent |
                boost::program_options::command_line_style::allow_long_disguise )
        .allow_unregistered().run();
    boost::program_options::variables_map variablesMap;
    store(parsed, variablesMap);
    notify(variablesMap);
    commandLineOptions.getUnrecognizedCollectionRef()
        = collect_unrecognized(parsed.options, boost::program_options::include_positional);;
    commandLineOptions.initCommandLine( commandLine );
}

void LintCombine::moveCommandLineToSTLContainer( stringVector & commandLine, const int argc, char ** argv ) {
    for( auto i = 1; i < argc; ++i ) {
        commandLine.emplace_back( argv[i] );
    }
}
