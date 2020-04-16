#ifndef __CLAZYWRAPPER_H__
#define __CLAZYWRAPPER_H__

#include "LinterWrapperBase.h"

#include <string>

class ClazyWrapper final : public LinterWrapperBase {
public:
    ClazyWrapper( const std::string & linterOptions, const std::string & yamlFilePath )
            : LinterWrapperBase ( linterOptions, yamlFilePath ) {
        linterName = "clazy-standalone";
    }

private:
    void addDocLinkToYaml( const YAML::Node & yamlNode ) override;
};

#endif //__CLAZYWRAPPER_H__
