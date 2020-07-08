#pragma once

#include "LinterFactoryBase.h"
#include "LinterItf.h"
#include "yaml-cpp/yaml.h"

#include <boost/process.hpp>

namespace LintCombine {
    class LinterBase : public LinterItf {

    public:
        void callLinter() override;

        int waitLinter() override;

        CallTotals updateYaml() const override;

        const std::string & getName() const;

        const std::string & getOptions() const;

        const std::string & getYamlPath() final;

        ~LinterBase() override = default;

    protected:
        explicit LinterBase( LinterFactoryBase::Services & service );

        explicit LinterBase( const stringVector & commandLine,
                             LinterFactoryBase::Services & service );

        void parseCommandLine( const stringVector & commandLine );

        virtual void updateYamlAction( const YAML::Node & yamlNode ) const = 0;

        std::string name;
        std::string options;
        std::string yamlPath;
        boost::process::child linterProcess;
        boost::process::async_pipe stdoutPipe;
        boost::process::async_pipe stderrPipe;

    private:
        // Buffer for reading from pipes
        std::array < char, 64 > m_buffer = {};

        void checkYamlPathForCorrectness();

        void readFromPipeToStream( boost::process::async_pipe & pipe, std::ostream & outputStream );
    };
}
