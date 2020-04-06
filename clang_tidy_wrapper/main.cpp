#include "yaml-cpp/yaml.h"

#include <fstream>
#include <iostream>
#include <string>

void clangTidySystemCalls() {
    // ToDo think about build dir's name
    std::system("mkdir -p _build && cd _build");
    std::system("cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..");
    // ToDo check clang-tidy version && output file's name
    std::system("clang-tidy-10 -checks=* -p ./ --export-fixes=clangTidyData.yaml ../main.cpp");
}

int main() {
    clangTidySystemCalls();
    YAML::Node config = YAML::LoadFile("clangTidyData.yaml");
    for(auto it: config["Diagnostics"]) {
        std::ostringstream oss;
        oss << "https://clang.llvm.org/extra/clang-tidy/checks/" << it["DiagnosticName"] << ".html";
        it["DiagnosticMessage"]["Documentation link"] = oss.str();
    }
    std::ofstream clangTidyWithDocLinkFile(CURRENT_SOURCE_DIR"/clangTidyWithDocLink.yaml");
    clangTidyWithDocLinkFile << config;
    return 0;
}
