#pragma once

#include "PrepareInputsBase.h"

namespace LintCombine {
    class PrepareInputsBareMSVC final : public PrepareInputsBase {
        void appendLintersOptionToCmdLine() override;

        bool isCalledExplicitly() override;
    };
}