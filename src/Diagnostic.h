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

        // if we need more operators in future we will use boost::operators
        friend bool operator<( const Diagnostic & lhs, const Diagnostic & rhs ) {
            if( lhs.level == rhs.level ) {
                return lhs.firstPos < rhs.firstPos;
            }
            return lhs.level < rhs.level;
        }

        bool operator==( const Diagnostic & rhs ) const {
            if( level == rhs.level &&
                origin == rhs.origin && firstPos == rhs.firstPos &&
                lastPos == rhs.lastPos && text == rhs.text ) {
                return true;
            }
            return false;
        }
    };
}