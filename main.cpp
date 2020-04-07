#include "yaml-cpp/yaml.h"

#include <boost/process.hpp>
#include <iostream>
#include <string>

void clangTidyCall(const std::string& commandLineString) {
    std::ostringstream oss;
    oss << "clang-tidy " << commandLineString;
    boost::process::system(oss.str());
}

void upgradeClangTidyOutputWithDocLink() {
    YAML::Node yalmFile = YAML::LoadFile("clangTidyYamlOutput.yaml");
    for(auto it: yalmFile["Diagnostics"]) {
        std::ostringstream oss;
        oss << "https://clang.llvm.org/extra/clang-tidy/checks/" << it["DiagnosticName"] << ".html";
        it["DiagnosticMessage"]["Documentation link"] = oss.str();
    }
    std::ofstream clangTidyWithDocLinkFile(CURRENT_SOURCE_DIR"/clangTidyYamlWithDocLink.yaml");
    clangTidyWithDocLinkFile << yalmFile;
}

int main(int argc, char* argv[]) {
    std::string commandLineString;
    for(int i = 1; i < argc; ++i) {
        commandLineString += argv[i];
        commandLineString += " ";
    }
    clangTidyCall(commandLineString);
    upgradeClangTidyOutputWithDocLink();
    return 0;
}
