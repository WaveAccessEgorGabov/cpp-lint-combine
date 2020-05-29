#pragma once

#include "FactoryBase.h"
#include "LinterBase.h"

namespace LintCombine {
    class ClazyWrapper final : public LinterBase {
    public:
        ClazyWrapper( stringVector && commandLineSTL, FactoryBase::Services & service );

    private:
        void updateYamlAction( const YAML::Node & yamlNode ) const final;

        static void addDocLinkToYaml( const YAML::Node & yamlNode );
    };
}
