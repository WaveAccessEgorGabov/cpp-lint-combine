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

        CallTotals updateYaml() override;

        const std::string & getName() const;

        const std::string & getOptions() const;

        const std::string & getYamlPath() final;

        std::vector< Diagnostic > diagnostics() override;

        ~LinterBase() override = default;

    protected:
        explicit LinterBase( LinterFactoryBase::Services & service );

        explicit LinterBase( const stringVector & cmdLine,
                             LinterFactoryBase::Services & service,
                             const std::string & nameVal );

        void parseCmdLine( const stringVector & cmdLine );

        virtual void updateYamlAction( const YAML::Node & yamlNode ) const = 0;

        std::string name;
        std::string options;
        std::string yamlPath;
        boost::process::child linterProcess;
        boost::process::async_pipe stdoutPipe;
        boost::process::async_pipe stderrPipe;
        std::vector< Diagnostic > m_diagnostics;

    private:
        // Buffer for reading from pipes
        std::array < char, 64 > m_buffer = {};

        void readFromPipeToStream( boost::process::async_pipe & pipe,
                                   std::ostream & outputStream );
    };
}
