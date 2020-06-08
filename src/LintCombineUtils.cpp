#include "LintCombineUtils.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string/replace.hpp>

void LintCombine::prepareCommandLineForReSharper(LintCombine::stringVector& commandLine) {
    boost::program_options::options_description programOptions;
    std::string pathToResultYaml;
    std::string pathToDiagnosticsDir;
    programOptions.add_options()
        ("export-fixes", boost::program_options::value < std::string >(&pathToResultYaml))
        ("p", boost::program_options::value < std::string >(&pathToDiagnosticsDir));
    const boost::program_options::parsed_options parsed =
        boost::program_options::command_line_parser(commandLine).options(programOptions)
        .style(boost::program_options::command_line_style::long_allow_adjacent |
             boost::program_options::command_line_style::allow_long_disguise)
        .allow_unregistered().run();
    boost::program_options::variables_map variablesMap;
    boost::program_options::store(parsed, variablesMap);
    notify(variablesMap);
    static std::vector < std::string > collectUnrecognized =
        boost::program_options::collect_unrecognized(parsed.options, boost::program_options::include_positional);
    commandLine.clear();
    commandLine.emplace_back("--result-yaml=" + pathToResultYaml);
    commandLine.emplace_back("--sub-linter=clang-tidy");
    // TODO: -line-filter - also add to clazy options
    for (size_t i = 0; i < collectUnrecognized.size(); ++i ) {
        boost::algorithm::replace_all(collectUnrecognized[i], "\"", "\\\"");
        std::string strInQuotes;
        if (collectUnrecognized[i].find("-config=") == 0 ) {
            strInQuotes = "-config";
        }
        if (collectUnrecognized[i].find("-line-filter=") == 0) {
            strInQuotes = "-line-filter=";
        }
        if( !strInQuotes.empty() ) {
            commandLine.emplace_back( strInQuotes + "\"" +
                collectUnrecognized[i].substr( std::string( strInQuotes ).size(), std::string::npos ) + "\"" );
            continue;
        }
        commandLine.emplace_back(collectUnrecognized[i]);
    }
    commandLine.emplace_back( "-p=" + pathToDiagnosticsDir);
    commandLine.emplace_back( "--export-fixes=" + pathToDiagnosticsDir + "\\diagnosticsClangTidy.yaml");
    commandLine.emplace_back( "--sub-linter=clazy");
    commandLine.emplace_back( "-p=" + pathToDiagnosticsDir);
    commandLine.emplace_back( "--export-fixes=" + pathToDiagnosticsDir + "\\diagnosticsClazy.yaml");
    // TODO: take diagnostics from config file
	commandLine.emplace_back( "-checks=level0,level1,level2");
    // TODO: parse path for clazy to file from command line
    commandLine.emplace_back( "E:\\WaveAccess\\WavePoint_General\\qt-clazy-test\\qt-clazy-test.cpp");
}

void LintCombine::moveCommandLineToSTLContainer( stringVector & commandLine, const int argc, char ** argv ) {
    for( auto i = 1; i < argc; ++i ) {
        commandLine.emplace_back( argv[i] );
    }
}
