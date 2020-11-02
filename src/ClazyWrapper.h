#pragma once

#include "LinterFactoryBase.h"
#include "LinterBase.h"

namespace LintCombine {
    class ClazyWrapper final : public LinterBase {
    public:
        ClazyWrapper( const StringVector & cmdLine, LinterFactoryBase::Services & service,
                      std::unique_ptr < LinterBehaviorItf > && linterBehaviorVal );

    private:
        void updateYamlData( YAML::Node & yamlNode ) const override;

        static void addDocLink( YAML::Node & yamlNode );
    };
}
