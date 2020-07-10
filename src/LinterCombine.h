#pragma once

#include "yaml-cpp/yaml.h"
#include "UsualLinterFactory.h"
#include "LinterItf.h"

#include <boost/program_options.hpp>
#include <memory>
#include <vector>

namespace LintCombine {
    class LinterCombine final : public LinterItf {

    public:
        explicit LinterCombine( const stringVector & commandLine,
                                LinterFactoryBase & factory = UsualLinterFactory::getInstance() );

        void callLinter() override;

        int waitLinter() override;

        const std::string & getYamlPath() override;

        CallTotals updateYaml() const override;

        std::shared_ptr < LinterItf > linterAt( size_t pos ) const;

        size_t numLinters() const noexcept;

        std::vector< Diagnostic > diagnostics() override;

    private:
        std::vector < stringVector > splitCommandLineBySubLinters( const stringVector & commandLine );

        void checkYamlPathForCorrectness();

        void mergeYaml( const std::string & yamlPathToMerge ) const;

        static YAML::Node loadYamlNode( const std::string & pathToYaml );

        std::vector < std::shared_ptr < LinterItf > > m_linters;
        std::string m_mergedYamlPath;
        LinterFactoryBase::Services & m_services;
        std::vector< Diagnostic > m_diagnostics;
    };
}
