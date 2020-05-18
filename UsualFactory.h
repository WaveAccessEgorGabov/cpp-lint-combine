#ifndef __STANDARDFACTORY_H__
#define __STANDARDFACTORY_H__

#include "FactoryBase.h"

namespace LintCombine {
    // UsualFactory is singleton
    class UsualFactory final : public FactoryBase {
    public:
        UsualFactory( UsualFactory & ) = delete;

        UsualFactory( UsualFactory && ) = delete;

        UsualFactory & operator=( UsualFactory const & ) = delete;

        UsualFactory & operator=( UsualFactory const && ) = delete;

        static UsualFactory & getInstance() {
            static UsualFactory instance;
            return instance;
        }

        std::shared_ptr < LinterItf > createLinter( int argc, char ** argv ) final;

    private:
        UsualFactory() = default;
    };
}

#endif //__STANDARDFACTORY_H__
