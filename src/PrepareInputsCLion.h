#pragma once

#include "PrepareInputsBase.h"

namespace LintCombine {
    class PrepareInputsCLion final : public PrepareInputsBase {

    public:
        void transformFiles() override;

    private:
        void appendLintersOptionToCmdLine() override;

    };
}
