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

        std::string getName() const;

        std::string getOptions() const;

        CallTotals getYamlPath( std::string & yamlPathOut ) final;

        std::vector< Diagnostic > diagnostics() const override;

        ~LinterBase() override = default;

    protected:
        std::string name;
        std::string yamlPath;

        explicit LinterBase( LinterFactoryBase::Services & service );

        explicit LinterBase( const StringVector & cmdLine,
                             LinterFactoryBase::Services & service,
                             const std::string & nameVal );

        void parseCmdLine( const StringVector & cmdLine );

        virtual void updateYamlData( YAML::Node & yamlNode ) const = 0;

    private:
        std::string m_options;
        boost::process::child m_linterProcess;
        boost::process::async_pipe m_stdoutPipe;
        boost::process::async_pipe m_stderrPipe;
        std::vector< Diagnostic > m_diagnostics;

        // Buffer for reading from pipes
        std::array< char, 64 > m_buffer = {};

        void readFromPipeToStream( boost::process::async_pipe & pipe,
                                   std::ostream & outputStream );
    };
}
