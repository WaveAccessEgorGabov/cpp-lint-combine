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

        std::unique_ptr< LinterItf > createLinter( const StringVector & cmdLine ) override;

    private:
        UsualLinterFactory() = default;
    };
}
