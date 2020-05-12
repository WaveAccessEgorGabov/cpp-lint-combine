#ifndef __LINTERWRAPPERBASE_H__
#define __LINTERWRAPPERBASE_H__

#include "FactoryBase.h"
#include "LinterItf.h"
#include "yaml-cpp/yaml.h"

#include <string>
#include <boost/asio.hpp>
#include <boost/process.hpp>

namespace LintCombine {
    class LinterBase : public LinterItf {
    public:
        void callLinter() override;

        int waitLinter() override;

        CallTotals updateYaml() override;

        const std::string & getName() const;

        const std::string & getOptions() const;

        const std::string & getYamlPath() const;

        virtual ~LinterBase() = default;

    protected:

        LinterBase( FactoryBase::Services & service );

        virtual void updateYamlAction( const YAML::Node & yamlNode ) = 0;

        virtual void parseCommandLine( int argc, char ** argv ) = 0;

        bool isNeedHelp;
        std::string name;
        std::string options;
        std::string yamlPath;
        FactoryBase::Services & service;
        boost::process::child linterProcess;
        boost::process::async_pipe stdoutPipe;
        boost::process::async_pipe stderrPipe;
        std::function < void( boost::process::async_pipe &, const int i ) > readFromPipeToStream;
    };
}

#endif //__LINTERWRAPPERBASE_H__
