#include "lintWrapperUtils.h"

#include <iostream>
#include <string>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    std::string linterName;
    std::string yamlFilePath;
    std::string linterOptions;
    try {
        if(parseCommandLine(argc, argv, linterName, yamlFilePath, linterOptions))
            return 0;
    }
    catch (const po::error& ex) {
        std::cerr << "Exception while parse command line; what(): " << ex.what() << std::endl;
        return 1;
    }

    if(linterName.empty()) {
        std::cerr << "Expected: linter exist" << std::endl;
        return 1;
    }

    const int linterReturnCode = callLinter(linterName, yamlFilePath, linterOptions);
    if(linterReturnCode) {
        std::cerr << "Error while running linter" << std::endl;
        return linterReturnCode;
    }
    if(!yamlFilePath.empty()) {
        if (!addDocLinkToYAMLFile(linterName, yamlFilePath)) {
            std::cerr << "Error while updating .yaml" << std::endl;
            return 1;
        }
    }

    return 0;
}
