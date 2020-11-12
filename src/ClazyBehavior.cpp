#include "ClazyBehavior.h"
#include <regex>

std::streamsize LintCombine::ClazyBehavior::convertLinterOutput( std::string & linterOutputPart ) {
    static const std::string rowColumn = "\\((\\d+),(\\d+)\\):";
    static const std::string path = "((?:[\\\\/][^\"*?\\/<>:|]+)+[^(" + rowColumn + "))])";
    static const std::string levelAndMessage = "( (?:warning|error): .* \\[)";
    static const std::string hyphens = "(-)*";
    static const std::string name = "(.*\\])";
    static const std::regex s_conform(
        path + "(?:" + rowColumn + levelAndMessage + hyphens + name + ")?"
    );
    std::smatch match;
    std::string wantedMessagePart;
    std::string convertedOutput;
    while( std::regex_search( linterOutputPart, match, s_conform ) ) {
        if( match[match.size() - 1].matched ) {
            convertedOutput += wantedMessagePart;
            wantedMessagePart.clear();
            convertedOutput += match.prefix().str();
            convertedOutput += std::regex_replace( match[0].str(), s_conform, "$1:$2:$3:$4$6" );
            linterOutputPart = match.suffix().str();
        }
        else if( match[1].matched ) {
            wantedMessagePart += match.prefix().str() + match.str();
            linterOutputPart = match.suffix().str();
        }
    }

    if( !wantedMessagePart.empty() ) {
        linterOutputPart = convertedOutput + wantedMessagePart + linterOutputPart;
        return convertedOutput.size();
    }

    if( !convertedOutput.empty() ) {
        linterOutputPart = convertedOutput + linterOutputPart;
        return linterOutputPart.size();
    }

    return -1;
}
