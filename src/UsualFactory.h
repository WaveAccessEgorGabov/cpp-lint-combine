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

        std::shared_ptr < LinterItf > createLinter( const stringVector & subLinterCommandLine ) override;

    private:
        UsualFactory() = default;
    };
}
