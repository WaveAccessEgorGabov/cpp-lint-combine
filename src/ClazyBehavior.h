#pragma once

#include "LinterBehaviorItf.h"

namespace LintCombine {
    class ClazyBehavior final : public LinterBehaviorItf {
    public:
        std::string convertLinterOutput( std::string && linterOutputPart,
                                         const ReadLinterOutputFrom readFrom ) override;

    private:
        std::string m_stdoutBuffer;
        std::string m_stderrBuffer;

        const std::string m_rowColumnDetector =
        #ifdef WIN32
            "([a-zA-Z]:([\\\\/][^\\\"\\*\\?\\\\/<>:|]*)*)" // 1: match windows-like file path
        #else
            "((/.*)*)" // match unix-like file path
        #endif
            "("
            "\\("    // match row/column part in clazy style
            "(\\d+)" // match row number
            ","
            "(\\d+)" // match column number
            "\\)"
            "):";

        const std::string m_checkNameDetector =
            m_rowColumnDetector +
            "( "
            "(warning|error): " // match check's level
            ".* "               // match check's message
            "\\[)(-)*(.*\\])";  // match check's name
    };
}