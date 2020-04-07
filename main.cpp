#include "yaml-cpp/yaml.h"

#include <boost/process.hpp>
#include <iostream>
#include <string>

static const int callClangTidy(const std::string& commandLineParameters) {
    std::string clangTidyExecutableCommand("clang-tidy ");
    clangTidyExecutableCommand.append(commandLineParameters);
    boost::process::child clangTidyProcess(clangTidyExecutableCommand);
    clangTidyProcess.wait();
    return clangTidyProcess.exit_code();
}

static bool addDocLinkToYAMLFile() {
    YAML::Node yamlNode;
    try {
        yamlNode = YAML::LoadFile("clangTidyYamlOutput.yaml");
    }
    catch (const YAML::BadFile& ex) {
        std::cerr << "Exception into addDocLinkToYAMLFile(); " << "what(): " << ex.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cerr << "Exception into addDocLinkToYAMLFile(); Error while load .yaml" << std::endl;
        return false;
    }

    for(auto it: yamlNode["Diagnostics"]) {
        std::ostringstream documentationLink;
        documentationLink << "https://clang.llvm.org/extra/clang-tidy/checks/" << it["DiagnosticName"] << ".html";
        it["DiagnosticMessage"]["Documentation link"] = documentationLink.str();
    }

    try {
        std::ofstream clangTidyWithDocLinkFile(CURRENT_SOURCE_DIR"/clangTidyYamlWithDocLink.yaml");
        clangTidyWithDocLinkFile << yamlNode;
    }
    catch (const std::ios_base::failure& ex) {
        std::cerr << "Exception into addDocLinkToYAMLFile(); " << "what(): " << ex.what() << std::endl;
        return  false;
    }
    catch (...) {
        std::cerr << "Exception into addDocLinkToYAMLFile(); Error while write to .yaml" << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    std::string commandLineParameters;
    for(int i = 1; i < argc; ++i) {
        commandLineParameters.append(argv[i]);
        commandLineParameters.append(" ");
    }

    const int clangTidyReturnCode = callClangTidy(commandLineParameters);
    if(clangTidyReturnCode) {
        return clangTidyReturnCode;
    }

    if(!addDocLinkToYAMLFile()) {
        return 1;
    }
    return 0;
}
