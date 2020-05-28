#pragma once

#include "yaml-cpp/yaml.h"
#include "UsualFactory.h"
#include "LinterItf.h"

#include <memory>
#include <vector>

namespace LintCombine {
    class LinterCombine final : public LinterItf {
    public:
        explicit LinterCombine( stringVectorConstRef commandLineSTL,
                                FactoryBase & factory = UsualFactory::getInstance() );

        explicit LinterCombine( int argc, char ** argv, FactoryBase & factory = UsualFactory::getInstance() );

        void callLinter() final;

        int waitLinter() final;

        const std::string & getYamlPath() final;

        CallTotals updateYaml() const final;

        std::shared_ptr < LinterItf > linterAt( int pos ) const;

        size_t numLinters() const noexcept;

        bool printTextIfRequested() const;

    private:
        std::vector < std::vector < std::string > > splitCommandLineBySubLinters( int argc, char ** argv );

        void mergeYaml( const std::string & yamlPathToMerge ) const;

        static char ** vectorStringToCharPP( const std::vector < std::string > & stringVector );

        static YAML::Node loadYamlNode( const std::string & pathToYaml );

        std::vector < std::shared_ptr < LinterItf > > m_linters;
        std::string m_mergedYamlPath;
        FactoryBase::Services & services;
        bool m_helpIsRequested = false;
    };
}
