#pragma once

#include "LinterFactoryBase.h"
#include "LinterBase.h"

namespace LintCombine {
    class ClangTidyWrapper final : public LinterBase {

    public:
        ClangTidyWrapper( const stringVector & cmdLine, LinterFactoryBase::Services & service );

    private:
        void updateYamlData( YAML::Node & yamlNode ) const override;

        static void addDocLink( YAML::Node & yamlNode );
    };
}
