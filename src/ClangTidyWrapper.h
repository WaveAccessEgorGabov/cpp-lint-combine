#pragma once

#include "LinterFactoryBase.h"
#include "LinterBase.h"

namespace LintCombine {
    class ClangTidyWrapper final : public LinterBase {

    public:
        ClangTidyWrapper( stringVector && commandLine, LinterFactoryBase::Services & service );

    private:
        void updateYamlAction( const YAML::Node & yamlNode ) const override;

        static void addDocLinkToYaml( const YAML::Node & yamlNode );
    };
}
