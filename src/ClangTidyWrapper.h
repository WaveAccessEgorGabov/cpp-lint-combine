#pragma once

#include "FactoryBase.h"
#include "LinterBase.h"

namespace LintCombine {
    class ClangTidyWrapper final : public LinterBase {
    public:
        ClangTidyWrapper( stringVector && commandLine, FactoryBase::Services & service );

    private:
        void updateYamlAction( const YAML::Node & yamlNode ) const final;

        static void addDocLinkToYaml( const YAML::Node & yamlNode );
    };
}
