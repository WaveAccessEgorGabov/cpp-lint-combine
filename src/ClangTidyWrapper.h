#pragma once

#include "FactoryBase.h"
#include "LinterBase.h"

namespace LintCombine {
    class ClangTidyWrapper final : public LinterBase {
    public:
        ClangTidyWrapper( int argc, char ** argv, FactoryBase::Services & service );

        ClangTidyWrapper( stringVectorConstRef commandLineSTL, FactoryBase::Services & service );

    private:
        void updateYamlAction( const YAML::Node & yamlNode ) const final;

        void parseCommandLine( stringVectorConstRef commandLineSTL ) final;

        void parseCommandLine( int argc, char ** argv ) final;

        static void addDocLinkToYaml( const YAML::Node & yamlNode );
    };
}
