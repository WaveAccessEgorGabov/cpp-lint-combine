#pragma once

#include <string>

namespace LintCombine {
    enum class Level {
        Trace, Debug, Info, Warning, Error, Fatal
    };

    struct Diagnostic {
        Diagnostic( std::string && textVal, const std::string && originVal,
                    const Level levelVal, const unsigned firstPosVal,
                    const unsigned lastPosVal )
            : text( textVal ), origin( originVal ), level( levelVal ),
            firstPos( firstPosVal ), lastPos( lastPosVal ) {}
        Level level;
        std::string text;
        std::string origin;
        unsigned firstPos;
        unsigned lastPos;
    };
}