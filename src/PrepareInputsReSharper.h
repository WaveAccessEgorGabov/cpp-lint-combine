#pragma once

#include "PrepareInputsBase.h"

namespace LintCombine {
    class PrepareInputsReSharper final : public PrepareInputsBase {
        void appendLintersOptionToCmdLine() override;
    };
}
