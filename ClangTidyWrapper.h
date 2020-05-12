#ifndef __CLANGTIDYWRAPPER_H__
#define __CLANGTIDYWRAPPER_H__

#include "FactoryBase.h"
#include "LinterBase.h"

namespace LintCombine {
    class ClangTidyWrapper final : public LinterBase {
    public:
        ClangTidyWrapper( int argc, char ** argv, FactoryBase::Services & service );

    private:
        void updateYamlAction( const YAML::Node & yamlNode ) const final;

        void addDocLinkToYaml( const YAML::Node & yamlNode ) const;

        void parseCommandLine( int argc, char ** argv ) final;
    };
}
#endif //__CLANGTIDYWRAPPER_H__
