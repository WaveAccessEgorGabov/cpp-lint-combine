#pragma once

#include "LinterFactoryBase.h"
#include "LinterBase.h"

namespace LintCombine {
    class ClazyWrapper final : public LinterBase {

    public:
        ClazyWrapper( stringVector && commandLineSTL, LinterFactoryBase::Services & service );

    private:
        void updateYamlAction( const YAML::Node & yamlNode ) const override;

        static void addDocLinkToYaml( const YAML::Node & yamlNode );
    };
}
