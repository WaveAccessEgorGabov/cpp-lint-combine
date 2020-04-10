#include "yaml-cpp/yaml.h"

#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace po = boost::program_options;

bool parseCommandLine(const int argc, char** argv, std::string& linterName,
                      std::string& yamlFileName, std::string& linterOptions) {
    po::options_description programOptions;
    programOptions.add_options()
            ("help,h", "Display available options")
            ("linter,L", po::value<std::string>(&linterName),
             "Linter name: clang-tidy, clazy-standalone")
            ("export-fixes", po::value<std::string>(&yamlFileName),
             "linter parameter: YAML file to store suggested fixes in. The"
             "stored fixes can be applied to the input source"
             "code with clang-apply-replacements.");

    po::parsed_options parsed = po::command_line_parser(argc, argv).options(programOptions).allow_unregistered().run();
    po::variables_map vm;
    po::store(parsed, vm);
    notify(vm);
    std::vector<std::string> linterOptionsVec = po::collect_unrecognized(parsed.options, po::include_positional);
    if(vm.count("help")) {
        std::cout << programOptions;
        return true;
    }
    for(auto it: linterOptionsVec)
        linterOptions.append(it + " ");
    return false;
}

static int callLinter(const std::string& linterName, const std::string& yamlFilePath,
        const std::string& linterOptions) {
#ifdef WIN32
    std::string linterExecutableCommand(CLANGTIDY_PATH"clang-tidy.exe ");
#elif __linux__
    std::string linterExecutableCommand = linterName + " " + linterOptions;
    if(!yamlFilePath.empty())
        linterExecutableCommand.append(" --export-fixes=" + yamlFilePath);
#endif
    try {
        boost::process::child linterProcess(linterExecutableCommand);
        linterProcess.wait();
        return linterProcess.exit_code();
    }
    catch(const boost::process::process_error& ex) {
        std::cerr << "Exception while run linter; what(): " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Exception while run linter" << std::endl;
    }
    return 1;
}

static bool addDocLinkToYAMLFile(std::string& linterName, std::string& yamlFilePath) {
    YAML::Node yamlNode;
    try {
        yamlNode = YAML::LoadFile(yamlFilePath);
    }
    catch (const YAML::BadFile& ex) {
        std::cerr << "Exception while load .yaml; what(): " << ex.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cerr << "Exception while load .yaml" << std::endl;
        return false;
    }

    for (auto it: yamlNode["Diagnostics"]) {
        std::ostringstream documentationLink;
        if(linterName == "clang-tidy")
            documentationLink << "https://clang.llvm.org/extra/clang-tidy/checks/" << it["DiagnosticName"] << ".html";
        if(linterName == "clazy-standalone") {
            std::ostringstream tempOss;
            tempOss << it["DiagnosticName"];
            documentationLink << "https://github.com/KDE/clazy/blob/master/docs/checks/README-";
            // substr() from 6 to size() for skipping "clazy-" in DiagnosticName
            documentationLink << tempOss.str().substr(6, tempOss.str().size()) << ".md";
        }
        it["Documentation link"] = documentationLink.str();
    }

    try {
        std::ofstream yamlWithDocLinkFile(CURRENT_SOURCE_DIR"/linterYamlWithDocLink.yaml");
        yamlWithDocLinkFile << yamlNode;
    }
    catch (const std::ios_base::failure& ex) {
        std::cerr << "Exception while writing updated .yaml " << "what(): " << ex.what() << std::endl;
        return  false;
    }
    catch (...) {
        std::cerr << "Exception while writing updated .yaml " << std::endl;
        return false;
    }
    return true;
}

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
