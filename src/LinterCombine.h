#pragma once

#include "yaml-cpp/yaml.h"
#include "UsualFactory.h"
#include "LinterItf.h"

#include <boost/program_options.hpp>
#include <memory>
#include <vector>

namespace LintCombine {
    class LinterCombine final : public LinterItf {
    public:
        explicit LinterCombine( stringVectorConstRef commandLine,
                                FactoryBase & factory = UsualFactory::getInstance() );

        void callLinter() final;

        int waitLinter() final;

        const std::string & getYamlPath() final;

        CallTotals updateYaml() const final;

        std::shared_ptr < LinterItf > linterAt( int pos ) const;

        size_t numLinters() const noexcept;

        bool printTextIfRequested() const;

    private:
        std::vector < stringVector > splitCommandLineBySubLinters( stringVectorConstRef commandLine );

        void checkYamlPathForCorrectness();

        void mergeYaml( const std::string & yamlPathToMerge ) const;

        static YAML::Node loadYamlNode( const std::string & pathToYaml );

        std::vector < std::shared_ptr < LinterItf > > m_linters;
        std::string m_mergedYamlPath;
        FactoryBase::Services & services;
        bool m_helpIsRequested = false;
        // contain program options
        boost::program_options::options_description genericOptDesc;
    };
}
