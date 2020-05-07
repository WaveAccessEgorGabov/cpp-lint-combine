#ifndef __STANDARDFACTORY_H__
#define __STANDARDFACTORY_H__

#include "FactoryBase.h"

namespace LintCombine {
    class StandardFactory : public FactoryBase {
    public:
        static StandardFactory & getInstance();

        std::vector < std::shared_ptr < LinterItf>>
        createLinter( std::vector < std::pair < std::string, char ** >> lintersAndTheirOptions ) override;
    };
}

#endif //__STANDARDFACTORY_H__
