#pragma once

#include "PrepareCmdLineBase.h"

namespace LintCombine {
    class PrepareCmdLineReSharper final : public PrepareCmdLineBase {
        void initOptionsToSpecificIDE() override;
    };
}
