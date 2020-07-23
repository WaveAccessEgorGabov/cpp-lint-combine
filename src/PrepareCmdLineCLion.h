#pragma once

#include "PrepareCmdLineBase.h"

namespace LintCombine {
    class PrepareCmdLineCLion final : public PrepareCmdLineBase {
        void actionsForSpecificIDE() override;
    };
}
