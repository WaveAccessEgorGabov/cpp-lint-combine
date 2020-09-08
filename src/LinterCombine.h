#pragma once

#include "yaml-cpp/yaml.h"
#include "UsualLinterFactory.h"
#include "LinterItf.h"

#include <memory>
#include <vector>

namespace LintCombine {

    enum WaitLinterReturnCode {
        AllLintersSucceeded = 0, SomeLintersFailed = 2, AllLintersFailed = 3
    };

    class LinterCombine final : public LinterItf {

    public:
        explicit LinterCombine(
            const StringVector & cmdLine,
            LinterFactoryBase & factory = UsualLinterFactory::getInstance() );

        void callLinter() override;

        int waitLinter() override;

        std::string getYamlPath() override;

        CallTotals updateYaml() override;

        std::shared_ptr< LinterItf > linterAt( size_t pos ) const;

        size_t numLinters() const noexcept;

        std::vector< Diagnostic > diagnostics() const override;

    private:
        std::vector< StringVector > splitCmdLineBySubLinters( const StringVector & cmdLine );

        void initCombinedYamlPath( const StringVector & cmdLine );

        void combineYamlFiles( const std::string & yamlPathForAppend );

        YAML::Node loadYamlNode( const std::string & pathToYaml );

        std::vector< std::shared_ptr< LinterItf > > m_linters;
        std::string m_combinedYamlPath;
        LinterFactoryBase::Services & m_services;
        std::vector< Diagnostic > m_diagnostics;
    };
}
