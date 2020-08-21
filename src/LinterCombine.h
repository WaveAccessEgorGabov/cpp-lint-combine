#pragma once

#include "yaml-cpp/yaml.h"
#include "UsualLinterFactory.h"
#include "LinterItf.h"

#include <memory>
#include <vector>

namespace LintCombine {
    class LinterCombine final : public LinterItf {

    public:
        explicit LinterCombine(
            const stringVector & cmdLine,
            LinterFactoryBase & factory = UsualLinterFactory::getInstance() );

        void callLinter() override;

        int waitLinter() override;

        const std::string & getYamlPath() override;

        CallTotals updateYaml() override;

        std::shared_ptr < LinterItf > linterAt( size_t pos ) const;

        size_t numLinters() const noexcept;

        std::vector< Diagnostic > diagnostics() override;

        bool isErrorOccur() const {
            return m_errorOccur;
        }

    private:
        std::vector < stringVector > splitCommandLineBySubLinters( const stringVector & commandLine );

        void validateGeneralYamlPath( const stringVector & cmdLine );

        void mergeYaml( const std::string & yamlPathToMerge );

        YAML::Node loadYamlNode( const std::string & pathToYaml );

        std::vector < std::shared_ptr < LinterItf > > m_linters;
        std::string m_pathToGeneralYaml;
        LinterFactoryBase::Services & m_services;
        std::vector< Diagnostic > m_diagnostics;
        bool m_errorOccur = false;
    };
}
