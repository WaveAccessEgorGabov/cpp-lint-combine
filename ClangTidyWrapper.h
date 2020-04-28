#ifndef __CLANGTIDYWRAPPER_H__
#define __CLANGTIDYWRAPPER_H__

#include "LinterWrapperBase.h"

#include <string>

class ClangTidyWrapper final : public LinterWrapperBase {
public:
    explicit ClangTidyWrapper( const std::string & linterOptions, const std::string & yamlFilePath )
            : LinterWrapperBase( linterOptions, yamlFilePath ) {
#ifdef WIN32
    linterName = "clang-tidy.exe";
#elif  __linux__
    linterName = "clang-tidy";
#endif
}

private:
    void addDocLinkToYaml( const YAML::Node & yamlNode ) const override;
};

#endif //__CLANGTIDYWRAPPER_H__
