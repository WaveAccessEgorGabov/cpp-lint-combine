#pragma once

#include "FactoryBase.h"

namespace LintCombine {
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
        std::shared_ptr < LinterItf > createLinter( stringVectorConstRef commandLineSTL ) final;

    private:
        UsualFactory() = default;
    };
}
