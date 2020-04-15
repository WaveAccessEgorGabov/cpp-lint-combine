#ifndef __CLANGTIDYWRAPPER_H__
#define __CLANGTIDYWRAPPER_H__

#include "LinterWrapperBase.h"

#include <string>

class ClangTidyWrapper : public LinterWrapperBase {
public:
    ClangTidyWrapper( const std::string & linterOptions, const std::string & yamlFilePath )
            : LinterWrapperBase ( linterOptions, yamlFilePath ) {
        linterName = "clang-tidy";
    }

private:
    void addDocLinkToYaml( const YAML::Node & yamlNode ) override;
};

#endif //__CLANGTIDYWRAPPER_H__
