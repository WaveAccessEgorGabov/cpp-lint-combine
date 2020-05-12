#ifndef __CLANGTIDYWRAPPER_H__
#define __CLANGTIDYWRAPPER_H__

#include "FactoryBase.h"
#include "LinterBase.h"

namespace LintCombine {
    class ClangTidyWrapper : public LinterBase {
    public:
        ClangTidyWrapper( int argc, char ** argv, FactoryBase::Services & service );

    private:
        void updateYamlAction( const YAML::Node & yamlNode ) override;

        void addDocLinkToYaml( const YAML::Node & yamlNode );

        void parseCommandLine( int argc, char ** argv ) override;
    };
}
#endif //__CLANGTIDYWRAPPER_H__
