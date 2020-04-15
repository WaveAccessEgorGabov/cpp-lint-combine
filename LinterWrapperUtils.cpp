#include "LinterWrapperUtils.h"
#include "ClangTidyWrapper.h"
#include "ClazyWrapper.h"

#include <boost/program_options.hpp>
#include <string>
#include <vector>

namespace po = boost::program_options;

LinterWrapperBase* linterWrapperFactory( const std::string& linterName, const std::string& linterOptions, const std::string& yamlFilePath) {
    if(linterName == "clang-tidy")
        return new ClangTidyWrapper(linterName, linterOptions, yamlFilePath);
    if(linterName == "clazy-standalone")
        return new ClazyWrapper(linterName, linterOptions, yamlFilePath);
    return nullptr;
}

LinterWrapperBase* parseCommandLine( const int argc, char** argv) {
    std::string linterName;
    std::string linterOptions;
    std::string yamlFilePath;
    po::options_description programOptions;
    programOptions.add_options()
            ("linter,L", po::value<std::string>(&linterName),
             "Linter name: clang-tidy, clazy-standalone")
            ("export-fixes", po::value<std::string>(&yamlFilePath),
             "linter parameter: YAML file to store suggested fixes in. The"
             "stored fixes can be applied to the input source"
             "code with clang-apply-replacements.");

    po::parsed_options parsed = po::command_line_parser(argc, argv).options(programOptions).allow_unregistered().run();
    po::variables_map vm;
    po::store(parsed, vm);
    notify(vm);
    std::vector<std::string> linterOptionsVec = po::collect_unrecognized(parsed.options, po::include_positional);
    for(auto it: linterOptionsVec)
        linterOptions.append(it + " ");
    return linterWrapperFactory ( linterName, linterOptions, yamlFilePath );
}
