#pragma once

#include "LinterFactoryBase.h"
#include "LinterBase.h"

namespace LintCombine {
    class ClazyWrapper final : public LinterBase {

    public:
        ClazyWrapper( const StringVector & cmdLine, LinterFactoryBase::Services & service );

    private:
        void updateYamlData( YAML::Node & yamlNode ) const override;

        static void addDocLink( YAML::Node & yamlNode );
    };
}
