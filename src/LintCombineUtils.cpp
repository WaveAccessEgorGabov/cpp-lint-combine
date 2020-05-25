#include "LintCombineUtils.h"

#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;

char ** prepareCommandLine( int argc, char ** argv ) {
	po::options_description programOptions;
	std::string pathToResultYaml;
	std::string pathToDiagnosticsDir;
	programOptions.add_options()
		("export-fixes", po::value < std::string >( &pathToResultYaml ) )
		("p",            po::value < std::string >( &pathToDiagnosticsDir ) );
	const po::parsed_options parsed =
		po::command_line_parser(argc, argv).options(programOptions)
		.style(po::command_line_style::long_allow_adjacent | po::command_line_style::allow_long_disguise)
		.allow_unregistered().run();
	po::variables_map variablesMap;
	po::store(parsed, variablesMap);
	notify(variablesMap);
	// MayBe copy vector to deque, for fast push_front
	std::vector < std::string > linterOptionsVec = po::collect_unrecognized(parsed.options, po::include_positional);

	return nullptr;
}