#ifndef __CLAZYTIDYWRAPPER_H__
#define __CLAZYTIDYWRAPPER_H__

#include "FactoryBase.h"
#include "LinterBase.h"

namespace LintCombine {
    class ClazyWrapper final : public LinterBase {
    public:
        ClazyWrapper( int argc, char ** argv, FactoryBase::Services & service );

    private:
        void updateYamlAction( const YAML::Node & yamlNode ) const final;

        static void addDocLinkToYaml( const YAML::Node & yamlNode );

        void parseCommandLine( int argc, char ** argv ) final;
    };
}
#endif //__CLAZYTIDYWRAPPER_H__
