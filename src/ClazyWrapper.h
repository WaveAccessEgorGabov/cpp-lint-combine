#pragma once

#include "FactoryBase.h"
#include "LinterBase.h"

namespace LintCombine {
    class ClazyWrapper final : public LinterBase {
    public:
        ClazyWrapper( int argc, char ** argv, FactoryBase::Services & service );

        ClazyWrapper( stringVectorConstRef commandLineSTL, FactoryBase::Services & service );

    private:
        void updateYamlAction( const YAML::Node & yamlNode ) const final;

        void parseCommandLine( stringVectorConstRef commandLineSTL ) final;

        void parseCommandLine( int argc, char ** argv ) final;

        static void addDocLinkToYaml( const YAML::Node & yamlNode );
    };
}
