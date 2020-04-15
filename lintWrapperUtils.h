#ifndef LINTWRAPPER_LINTWRAPPERUTILS_H
#define LINTWRAPPER_LINTWRAPPERUTILS_H

#include <string>

bool parseCommandLine(const int argc, char** argv, std::string& linterName,
                             std::string& yamlFileName, std::string& linterOptions);

int callLinter(const std::string& linterName, const std::string& yamlFilePath,
                      const std::string& linterOptions);

bool addDocLinkToYAMLFile(std::string& linterName, std::string& yamlFilePath);

#endif //LINTWRAPPER_LINTWRAPPERUTILS_H
