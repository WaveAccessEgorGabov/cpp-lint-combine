#pragma once

#include "LinterFactoryBase.h"
#include "LinterItf.h"
#include "yaml-cpp/yaml.h"
#include "LinterBehaviorItf.h"

#include <boost/process.hpp>

#include <map>
#include <iostream>

namespace LintCombine {
    class LinterBase : public LinterItf {
    public:
        void callLinter( const std::unique_ptr< IdeBehaviorItf > & ideBehavior ) override;

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

        explicit LinterBase( LinterFactoryBase::Services & service,
                             std::unique_ptr< LinterBehaviorItf > && linterBehaviorVal );

        explicit LinterBase( const StringVector & cmdLine,
                             LinterFactoryBase::Services & service,
                             const std::string & nameVal,
                             std::unique_ptr< LinterBehaviorItf > && linterBehaviorVal );

        void parseCmdLine( const StringVector & cmdLine );

        virtual void updateYamlData( YAML::Node & yamlNode ) const = 0;

    private:
        std::string m_options;
        boost::process::child m_linterProcess;
        boost::process::async_pipe m_stdoutPipe;
        boost::process::async_pipe m_stderrPipe;
        std::vector< Diagnostic > m_diagnostics;
        std::unique_ptr < LinterBehaviorItf > m_linterBehavior;

        // Buffer for reading from pipes
        std::array< char, 512 > m_readPart{};
        std::map< std::ostream *, std::string > m_streamBufferMap = {
            { &std::cout, {} }, { &std::cerr, {} } };
        bool m_convertLinterOutput = false;

        void readFromPipeToStream( boost::process::async_pipe & pipe,
                                   std::ostream & outputStream );
    };
}
