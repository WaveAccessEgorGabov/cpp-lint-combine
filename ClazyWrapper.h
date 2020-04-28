#ifndef __CLAZYWRAPPER_H__
#define __CLAZYWRAPPER_H__

#include "LinterWrapperBase.h"

#include <string>

class ClazyWrapper final : public LinterWrapperBase {
public:
    explicit ClazyWrapper( const std::string & linterOptions, const std::string & yamlFilePath )
            : LinterWrapperBase( linterOptions, yamlFilePath ) {
#ifdef WIN32
    linterName = "clazy-standalone.exe";
#elif __linux__
    linterName = "clazy-standalone";
#endif
    }


private:
    void addDocLinkToYaml( const YAML::Node & yamlNode ) const override;
};

#endif //__CLAZYWRAPPER_H__
