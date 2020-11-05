#pragma once

#include "LinterBehaviorItf.h"

#include <regex>

namespace LintCombine {
    class ClazyBehavior final : public LinterBehaviorItf {
    public:
        std::string convertLinterOutput( std::string && linterOutputPart,
                                         const ReadLinterOutputFrom readFrom ) override;

    private:
        std::string m_stdoutBuffer;
        std::string m_stderrBuffer;

        const std::regex m_regexForConversion{
        #ifdef WIN32
            "([a-zA-Z]:([\\\\/][^\\\"\\*\\?\\\\/<>:|]*)*)" // 1: match windows-like file path
        #else
            "((/.*)*)" // 1: match unix-like file path
        #endif
            "("      // match row/column part in clazy style
            "\\("
            "(\\d+)" // 4: match row number
            ","
            "(\\d+)" // 5: match column number
            "\\)"
            "):"
            "( "                //
            "(warning|error): " // 6: match check's level
            ".* "               //    match check's message
            "\\["               //
            ")"                 //
            ""
            "(-)*"              // match hyphens in the check's name begin
            "(.*\\])", std::regex::optimize };  // 9: match check's name
    };
}