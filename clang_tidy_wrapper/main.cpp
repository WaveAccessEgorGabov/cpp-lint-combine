#include "yaml-cpp/yaml.h"

#include <boost/process.hpp>
#include <iostream>
#include <string>

void clangTidyCall(const std::string& commandLineString) {
    std::ostringstream oss;
    oss << "clang-tidy-10 " << commandLineString;
    boost::process::ipstream clangTidyOutput;
    boost::process::ipstream clangTidyError;
    boost::process::system(oss.str(), boost::process::std_out > clangTidyOutput,
                                      boost::process::std_err > clangTidyError);
    std::cout << "Clang-Tidy output: " << std::endl << clangTidyOutput.rdbuf();
}

int main(int argc, char* argv[]) {
    std::string commandLineString;
    for(int i = 1; i < argc; ++i) {
        commandLineString += argv[i];
        commandLineString += " ";
    }
    clangTidyCall(commandLineString);

    YAML::Node config = YAML::LoadFile("clangTidyYamlOutput.yaml");
    for(auto it: config["Diagnostics"]) {
        std::ostringstream oss;
        oss << "https://clang.llvm.org/extra/clang-tidy/checks/" << it["DiagnosticName"] << ".html";
        it["DiagnosticMessage"]["Documentation link"] = oss.str();
    }
    std::ofstream clangTidyWithDocLinkFile(CURRENT_SOURCE_DIR"/clangTidyYamlWithDocLink.yaml");
    clangTidyWithDocLinkFile << config;
    return 0;
}
