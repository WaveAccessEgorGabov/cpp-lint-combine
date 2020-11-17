#pragma once

#include "yaml-cpp/yaml.h"
#include "UsualLinterFactory.h"
#include "LinterItf.h"

#include <memory>
#include <vector>

namespace LintCombine {
    enum RetCode {
        RC_Success = 0, RC_PartialFailure = 2, RC_TotalFailure = 3
    };

    class LinterCombine final : public LinterItf {
    public:
        explicit LinterCombine(
            const StringVector & cmdLine,
            LinterFactoryBase & factory = UsualLinterFactory::getInstance() );

        void callLinter( const std::unique_ptr< IdeBehaviorItf > & ideBehavior ) override;

        // Returns: 0 if successful, implementation-defined nonzero value otherwise.
        int waitLinter() override;

        CallTotals getYamlPath( std::string & yamlPathOut ) override;

        CallTotals updateYaml() override;

        std::shared_ptr< LinterItf > linterAt( size_t pos ) const;

        size_t numLinters() const noexcept;

        std::vector< Diagnostic > diagnostics() const override;

    private:
        std::vector< StringVector > splitCmdLineBySubLinters( const StringVector & cmdLine ) const;

        void initCombinedYamlPath( const StringVector & cmdLine );

        // Allowing YAML-files combination are:
        // 1. Path to result YAML-file set && At least one linter set path to YAML-file
        // 2. Path to result YAML-file not set && No linter's path to YAML-file set
        void checkIsRequiredYamlFilesCombinationSpecified();

        void combineYamlFiles( const std::string & yamlPathToAppend );

        YAML::Node loadYamlNode( const std::string & pathToYaml );

        std::vector< std::shared_ptr< LinterItf > > m_linters;
        std::string m_combinedYamlPath;
        LinterFactoryBase::Services & m_services;
        std::vector< Diagnostic > m_diagnostics;
        bool m_alreadyTriedToGetYamlPath = false;
    };
}
