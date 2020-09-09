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

    // if we need more operators in future we will use boost::operators
    inline bool operator<( const Diagnostic & lhs, const Diagnostic & rhs ) {
        if( lhs.level == rhs.level )
            return lhs.firstPos < rhs.firstPos;
        return lhs.level < rhs.level;
    }

    inline bool operator==( const Diagnostic & lhs, const Diagnostic & rhs ) {
        return lhs.level == rhs.level &&
               lhs.origin == rhs.origin && lhs.firstPos == rhs.firstPos &&
               lhs.lastPos == rhs.lastPos && lhs.text == rhs.text;
    }
}