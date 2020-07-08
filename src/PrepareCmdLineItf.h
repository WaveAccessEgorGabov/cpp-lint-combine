#pragma once

#include "LinterItf.h"

namespace LintCombine {

    enum class Level {
        Trace, Debug, Info, Warning, Error, Fatal
    };

    struct PrepareCmdLineItf {
        virtual ~PrepareCmdLineItf() = default;

        struct Diagnostic {
            Diagnostic( std::string && textVal, const Level levelVal,
                        const unsigned firstPosVal, const unsigned lastPosVal )
                : text( textVal ), level( levelVal ),
                firstPos( firstPosVal ), lastPos( lastPosVal ) {}
            std::string text;
            Level level;
            unsigned firstPos;
            unsigned lastPos;
        };

        virtual stringVector transform( stringVector commandLine ) = 0;

        virtual std::vector< Diagnostic > diagnostics() = 0;
    };
}
