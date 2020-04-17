#ifndef __LINTERWRAPPERBASE_H__
#define __LINTERWRAPPERBASE_H__

#include "LinterWrapperItf.h"
#include "yaml-cpp/yaml.h"

#include <string>

class LinterWrapperBase : public LinterWrapperItf {
public:
    [[nodiscard]] int callLinter( bool isNeedHelp = false ) const override;

    [[nodiscard]] bool createUpdatedYaml() const override;

    [[nodiscard]] const std::string & getLinterName() const;

    [[nodiscard]] const std::string & getLinterOptions() const;

    [[nodiscard]] const std::string & getYamlFilePath() const;

protected:

    explicit LinterWrapperBase( const std::string & linterOptions, const std::string & yamlFilePath )
            : linterOptions( linterOptions ), yamlFilePath( yamlFilePath ) {
    }

    virtual void addDocLinkToYaml( const YAML::Node & yamlNode ) const = 0;

    std::string linterName;
    std::string linterOptions;
    std::string yamlFilePath;
};

#endif //__LINTERWRAPPERBASE_H__
