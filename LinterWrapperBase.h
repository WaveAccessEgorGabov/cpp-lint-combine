#ifndef __LINTERWRAPPER_H__
#define __LINTERWRAPPER_H__

#include "LinterWrapperItf.h"
#include "yaml-cpp/yaml.h"

#include <string>

class LinterWrapperBase : public LinterWrapperItf {
public:
    int callLinter() override;

    bool createUpdatedYaml() override;

    const std::string & getLinterName() const;

    const std::string & getLinterOptions() const;

    const std::string & getYamlFilePath() const;

protected:

    LinterWrapperBase( const std::string & linterOptions, const std::string & yamlFilePath )
            : linterOptions( linterOptions ), yamlFilePath( yamlFilePath ) {}

    virtual void addDocLinkToYaml( const YAML::Node & yamlNode ) = 0;

    std::string linterName;
    std::string linterOptions;
    std::string yamlFilePath;
};

#endif //__LINTERWRAPPER_H__
