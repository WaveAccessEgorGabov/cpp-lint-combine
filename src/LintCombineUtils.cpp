#include "LintCombineUtils.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace po = boost::program_options;

void LintCombine::prepareCommandLineForReSharper(LintCombine::stringVector& commandLineSTL)
{
	po::options_description programOptions;
	std::string pathToResultYaml;
	std::string pathToDiagnosticsDir;
	programOptions.add_options()
        ("export-fixes", po::value < std::string >(&pathToResultYaml))
        ("p", po::value < std::string >(&pathToDiagnosticsDir));
	const po::parsed_options parsed = po::command_line_parser(commandLineSTL).options(programOptions)
        .style(po::command_line_style::long_allow_adjacent | po::command_line_style::allow_long_disguise)
        .allow_unregistered().run();
	po::variables_map variablesMap;
	po::store(parsed, variablesMap);
	notify(variablesMap);
	static std::vector < std::string > collectUnrecognized
        = po::collect_unrecognized(parsed.options, po::include_positional);
	commandLineSTL.clear();
	commandLineSTL.emplace_back("--result-yaml=" + pathToResultYaml);
	commandLineSTL.emplace_back("--sub-linter=clang-tidy");
	boost::algorithm::replace_all( collectUnrecognized[ collectUnrecognized.size() - 1 ], "\"", "\\\"" );
	boost::algorithm::replace_all(collectUnrecognized[collectUnrecognized.size() - 1], "[", "\"[");
	boost::algorithm::replace_all(collectUnrecognized[collectUnrecognized.size() - 1], "]", "\"]");
	for (const auto& it : collectUnrecognized) {
		commandLineSTL.emplace_back(it);
	}
	commandLineSTL.emplace_back( "-p=" + pathToDiagnosticsDir);
	commandLineSTL.emplace_back( "--export-fixes=" + pathToDiagnosticsDir + "\\diagnosticsClangTidy.yaml");
	commandLineSTL.emplace_back( "--sub-linter=clazy");
	commandLineSTL.emplace_back( "-p=" + pathToDiagnosticsDir);
	commandLineSTL.emplace_back( "--export-fixes=" + pathToDiagnosticsDir + "\\diagnosticsClazy.yaml");
	commandLineSTL.emplace_back( "-checks=level0,level1,level2");
	// TODO: parse path for clazy to file from command line
	commandLineSTL.emplace_back( "E:\\WaveAccess\\WavePoint_General\\qt-clazy-test\\qt-clazy-test.cpp");
}

void LintCombine::moveCommandLineToSTLContainer( stringVector & commandLineSTL, int argc, char ** argv ) {
    for( int i = 1; i < argc; ++i ) {
        commandLineSTL.emplace_back( argv[i] );
    }
}
