#include "ClazyBehavior.h"
#include <regex>

std::string LintCombine::ClazyBehavior::convertLinterOutput( std::string && linterOutputPart,
                                                             const ReadLinterOutputFrom readFrom ) {
    static const std::regex s_conform{
        #ifdef WIN32
            // for UNC path starts with '\\'
            R"(([a-zA-Z]:(?:[\\/][^\"\\*\\?\\/<>:|]*)*))" // 1: match windows-like file path
        #else
            "((/.*)*)" // 1: match unix-like file path
        #endif
            "(?:"                 // match row/column part in clazy style
            "\\("
            "(\\d+)"              // 2: match row number
            ","
            "(\\d+)"              // 3: match column number
            "\\)"
            "):"
            "( "                  //
            "(?:warning|error): " // 4: match check's level
            ".* "                 //    match check's message
            "\\["                 //
            ")"                   //
            "(-)*"                // match hyphens in the check's name begin
            "(.*\\])",            // 5: match check's name
            std::regex::optimize };

    std::string conversionResult;
    auto & currentWorkBuffer =
        readFrom == ReadLinterOutputFrom::Stdout ? m_stdoutBuffer : m_stderrBuffer;
    std::string::size_type pos = 0;
    if( ( pos = linterOutputPart.find( "\n" ) ) == std::string::npos ) {
        currentWorkBuffer += linterOutputPart;
        return conversionResult;
    }

    currentWorkBuffer += linterOutputPart.substr( 0, pos + 1 );
    while( pos != std::string::npos ) {
        conversionResult +=
            std::regex_replace( currentWorkBuffer, s_conform, "$1:$2:$3:$4$6" );

        const auto oldPos = pos;
        pos = linterOutputPart.find( "\n", pos + 1 );
        if( pos == std::string::npos ) {
            currentWorkBuffer = linterOutputPart.substr( oldPos + 1 );
        }
        if( pos != std::string::npos ) {
            currentWorkBuffer = linterOutputPart.substr( oldPos + 1, pos - oldPos );
        }
    }
    return conversionResult;
}
