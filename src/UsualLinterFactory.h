#pragma once

#include "LinterFactoryBase.h"

namespace LintCombine {
    class UsualLinterFactory final : public LinterFactoryBase {

    public:
        UsualLinterFactory( UsualLinterFactory & ) = delete;

        UsualLinterFactory( UsualLinterFactory && ) = delete;

        UsualLinterFactory & operator=( UsualLinterFactory const & ) = delete;

        UsualLinterFactory & operator=( UsualLinterFactory const && ) = delete;

        static UsualLinterFactory & getInstance() {
            static UsualLinterFactory instance;
            return instance;
        }

        std::shared_ptr < LinterItf > createLinter( const stringVector & subLinterCmdLine ) override;

    private:
        UsualLinterFactory() = default;
    };
}
