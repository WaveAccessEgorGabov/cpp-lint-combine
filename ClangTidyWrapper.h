#ifndef __CLANGTIDYWRAPPER_H__
#define __CLANGTIDYWRAPPER_H__

#include "LinterWrapperBase.h"

#include <string>

class ClangTidyWrapper : public LinterWrapperBase {
public:
    ClangTidyWrapper() = default;

    ClangTidyWrapper( const std::string & linterName, const std::string & linterOptions,
                      const std::string & yamlFilePath )
            : LinterWrapperBase ( linterName, linterOptions, yamlFilePath ) {}

private:
    void addDocLinkToYaml( const YAML::Node & yamlNode ) override;
};

#endif //__CLANGTIDYWRAPPER_H__
