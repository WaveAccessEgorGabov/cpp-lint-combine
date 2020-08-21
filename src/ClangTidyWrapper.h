#pragma once

#include "LinterFactoryBase.h"
#include "LinterBase.h"

namespace LintCombine {
    class ClangTidyWrapper final : public LinterBase {

    public:
        ClangTidyWrapper( const stringVector & cmdLine, LinterFactoryBase::Services & service );

    private:
        void updateYamlAction( const YAML::Node & yamlNode ) const override;

        static void addDocLink( const YAML::Node & yamlNode );
    };
}

