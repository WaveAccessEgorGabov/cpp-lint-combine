#ifndef __CLAZYWRAPPER_H__
#define __CLAZYWRAPPER_H__

#include "LinterWrapperBase.h"

#include <string>

class ClazyWrapper : public LinterWrapperBase {
public:
    ClazyWrapper() = default;

    ClazyWrapper( const std::string & linterName, const std::string & linterOptions, const std::string & yamlFilePath )
            : LinterWrapperBase ( linterName, linterOptions, yamlFilePath ) {}

private:
    void addDocLinkToYaml( const YAML::Node & yamlNode ) override;
};

#endif //__CLAZYWRAPPER_H__
