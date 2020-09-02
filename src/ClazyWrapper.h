#pragma once

#include "LinterFactoryBase.h"
#include "LinterBase.h"

namespace LintCombine {
    class ClazyWrapper final : public LinterBase {

    public:
        ClazyWrapper( const stringVector & cmdLine, LinterFactoryBase::Services & service );

    private:
        void updateYAMLAction( const YAML::Node & yamlNode ) const override;

        static void addDocLink( const YAML::Node & yamlNode );
    };
}
