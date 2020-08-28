#pragma once

#include <string>

namespace LintCombine {
    enum class Level {
        Trace, Debug, Info, Warning, Error, Fatal
    };

    struct Diagnostic {
        Diagnostic( const Level levelVal,
                    const std::string & textVal, const std::string & originVal,
                    const unsigned firstPosVal, const unsigned lastPosVal )
            : level( levelVal ), text( textVal ), origin( originVal ),
            firstPos( firstPosVal ), lastPos( lastPosVal ) {}
        Level level;
        std::string text;
        std::string origin;
        unsigned firstPos;
        unsigned lastPos;
    };
}