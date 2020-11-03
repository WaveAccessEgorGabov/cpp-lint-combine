#include "ClazyBehavior.h"

#include <regex>

std::string LintCombine::ClazyBehavior::convertLinterOutput( std::string && linterOutputPart,
                                                             const ReadLinterOutputFrom readFrom ) {
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
            std::regex_replace( currentWorkBuffer, std::regex( m_regexForConversion ), "$1:$4:$5:$6$9" );

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
