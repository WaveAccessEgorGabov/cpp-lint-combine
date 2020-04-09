#include "yaml-cpp/yaml.h"

#include <boost/process.hpp>
#include <fstream>
#include <iostream>
#include <string>

static int callClangTidy(const std::string& commandLineParameters) {
#ifdef WIN32
    std::string clangTidyExecutableCommand(CLANGTIDY_PATH"clang-tidy.exe ");
#elif __linux__
    std::string clangTidyExecutableCommand("clang-tidy ");
#endif
    clangTidyExecutableCommand.append(commandLineParameters);
    std::cout << clangTidyExecutableCommand << std::endl;
    try {
        boost::process::child clangTidyProcess(clangTidyExecutableCommand);
        clangTidyProcess.wait();
        return clangTidyProcess.exit_code();
    }
    catch(const boost::process::process_error& ex) {
        std::cerr << "Exception while run clang-tidy; what(): " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Exception while run clang-tidy" << std::endl;
    }
    return 1;
}

static bool addDocLinkToYAMLFile() {
    YAML::Node yamlNode;
    try {
        yamlNode = YAML::LoadFile("clangTidyYamlOutput.yaml");
    }
    catch (const YAML::BadFile& ex) {
        std::cerr << "Exception while load .yaml " << "what(): " << ex.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cerr << "Exception while load .yaml" << std::endl;
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
    std::string commandLineParameters;
    for(int i = 1; i < argc; ++i) {
        commandLineParameters.append(argv[i]);
        commandLineParameters.append(" ");
    }

    const int clangTidyReturnCode = callClangTidy(commandLineParameters);
    if(clangTidyReturnCode) {
        std::cerr << "Error while running clang-tidy" << std::endl;
        return clangTidyReturnCode;
    }

    if(!addDocLinkToYAMLFile()) {
        std::cerr << "Error while updating .yaml" << std::endl;
        return 1;
    }
    return 0;
}
