#include "yaml-cpp/yaml.h"

#include <boost/process.hpp>
#include <iostream>
#include <string>

bool clangTidyCall(const std::string& commandLineString) {
    std::ostringstream oss;
    oss << "clang-tidy " << commandLineString;
    if(!boost::process::system(oss.str()))
        return true;
    std::cerr << "Error in runtime clang-tidy" << std::endl;
    return false;
}

bool addDocLinkToYAMLFile() {
    YAML::Node yamlFile;
    try {
        yamlFile = YAML::LoadFile("clangTidyYamlOutput.yaml");
    }
    catch (const YAML::BadFile& ex) {
        std::cerr << "Exception into addDocLinkToYAMLFile(); " << "what(): " << ex.what() << std::endl;
        return false;
    }

    for(auto it: yamlFile["Diagnostics"]) {
        std::ostringstream oss;
        oss << "https://clang.llvm.org/extra/clang-tidy/checks/" << it["DiagnosticName"] << ".html";
        it["DiagnosticMessage"]["Documentation link"] = oss.str();
    }
    try {
        std::ofstream clangTidyWithDocLinkFile(CURRENT_SOURCE_DIR"/clangTidyYamlWithDocLink.yaml");
        clangTidyWithDocLinkFile << yamlFile;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception into addDocLinkToYAMLFile(); " << "what(): " << ex.what() << std::endl;
        return  false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    std::string commandLineString;
    for(int i = 1; i < argc; ++i) {
        commandLineString += argv[i];
        commandLineString += " ";
    }
    if(!clangTidyCall(commandLineString))
        return 1;
    if(!addDocLinkToYAMLFile())
        return 1;
    return 0;
}
