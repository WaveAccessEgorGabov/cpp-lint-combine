#ifndef __LINTERWRAPPER_H__
#define __LINTERWRAPPER_H__

#include "LinterWrapperItf.h"
#include "yaml-cpp/yaml.h"

#include <string>

class LinterWrapperBase : public LinterWrapperItf {
public:
    int callLinter() override;

    bool createUpdatedYaml() override;

protected:
    LinterWrapperBase() = default;

    LinterWrapperBase( const std::string & linterName, const std::string & linterOptions,
                       const std::string & yamlFilePath )
            : linterName ( linterName ), linterOptions ( linterOptions ), yamlFilePath ( yamlFilePath ) {}

    virtual void addDocLinkToYaml( const YAML::Node & yamlNode ) = 0;

    const std::string & getLinterName() const;

    const std::string & getLinterOptions() const;

    const std::string & getYamlFilePath() const;

    std::string linterName;
    std::string linterOptions;
    std::string yamlFilePath;
};

#endif //__LINTERWRAPPER_H__
