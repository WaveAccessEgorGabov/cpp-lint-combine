#ifndef __LINTERWRAPPERBASE_H__
#define __LINTERWRAPPERBASE_H__

#include "Factory.h"
#include "LinterWrapperItf.h"
#include "yaml-cpp/yaml.h"

#include <string>

namespace LintCombine {
    class LinterWrapperBase : public LinterWrapperItf {
    public:
        void callLinter() const override;

        int waitLinter() const override;

        CallTotals updatedYaml() const override;

        const std::string & getName() const;

        const std::string & getOptions() const;

        const std::string & getYamlPath() const;

    protected:

        virtual void updateYamlAction( const YAML::Node & yamlNode ) const = 0;

        virtual void parseCommandLine( int argc, char ** argv ) = 0;

        bool isNeedHelp;
        std::string name;
        std::string options;
        std::string yamlPath;
        Factory::Service service;
    };
}

#endif //__LINTERWRAPPERBASE_H__
