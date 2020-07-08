#pragma once

#include "PrepareCmdLineBase.h"

namespace LintCombine {
    class PrepareCmdLineReSharper final : public PrepareCmdLineBase {
    public:
        void appendOptionsToSpecificIDE() override;
    private:
        void initCommonOptions();

        void initUnrecognizedOptions();

        void addOptionToLinterByName( const std::string & name,
                                      const std::string & option );

        void addOptionToAllLinters( const std::string & option );

        static std::string
        optionValueToQuotes( const std::string & optionName,
                             const std::string & optionNameWithValue );

        void appendLintersOptionToCmdLine();
    };
}
