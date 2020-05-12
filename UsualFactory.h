#ifndef __STANDARDFACTORY_H__
#define __STANDARDFACTORY_H__

#include "FactoryBase.h"

namespace LintCombine {
    class UsualFactory : public FactoryBase {
    public:
        UsualFactory(UsualFactory&) = delete;

        UsualFactory(UsualFactory&&) = delete;

        UsualFactory& operator= (UsualFactory const&) = delete;

        UsualFactory& operator= (UsualFactory const&&) = delete;

        static UsualFactory & getInstance();

        std::shared_ptr < LinterItf >
        createLinter( std::vector < std::pair < std::string, char ** >> lintersAndTheirOptions );

    private:
        UsualFactory();
    };
}

#endif //__STANDARDFACTORY_H__
