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

        CallTotals updateYaml() const override;

        const std::string & getName() const;

        const std::string & getOptions() const;

        const std::string & getYamlPath() const;

        virtual ~LinterBase() = default;

    protected:

        explicit LinterBase( FactoryBase::Services & service );

        virtual void updateYamlAction( const YAML::Node & yamlNode ) const = 0;

        virtual void parseCommandLine( int argc, char ** argv ) = 0;

        std::string name;
        std::string options;
        std::string yamlPath;
        boost::process::child linterProcess;
        boost::process::async_pipe stdoutPipe;
        boost::process::async_pipe stderrPipe;
    private:
        FactoryBase::Services & service;
        std::array < char, 64 > buffer = {};
        void readFromPipeToStream( boost::process::async_pipe & pipe, std::ostream & stream );
    };
}

#endif //__LINTERWRAPPERBASE_H__
