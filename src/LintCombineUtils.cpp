#include "LintCombineUtils.h"
#include "LintCombineException.h"
#include "OutputHelper.h"

#include <fstream>
#include <algorithm>
#include <unordered_map>

#ifdef WIN32
#include <windows.h> // for WideCharToMultiByte(), MultiByteToWideChar() and GetLastError()

std::wstring LintCombine::Utf8ToUtf16( const std::string & utf8 ) {
    std::wstring utf16;

    if( utf8.empty() ) {
        return utf16;
    }

    // Safely fail if an invalid UTF-8 character sequence is encountered
    constexpr auto kFlags = MB_ERR_INVALID_CHARS;

    // Safely cast the length of the source UTF-8 string (expressed in chars)
    // from size_t (returned by std::string::length()) to int
    // for the MultiByteToWideChar API.
    // If the size_t value is too big to be stored into an int,
    // throw an exception to prevent conversion errors like huge size_t values
    // converted to *negative* integers.
    if( utf8.length() > static_cast< size_t >( ( std::numeric_limits<int>::max )( ) ) ) {
        throw Exception( { Level::Error,
                           "Linter command line too long. Max size is " +
                           std::to_string( ( std::numeric_limits<int>::max )( ) ),
                           "LintCombineUtils", 1, 0 } );
    }
    const auto utf8Length = static_cast< int >( utf8.length() );

    // Get the size of the destination UTF-16 string
    const int utf16Length = MultiByteToWideChar(
        CP_UTF8,       // source string is in UTF-8
        kFlags,        // conversion flags
        utf8.data(),   // source UTF-8 string pointer
        utf8Length,    // length of the source UTF-8 string, in chars
        nullptr,       // unused - no conversion done in this step
        0              // request size of destination buffer, in wchar_ts
    );
    if( utf16Length == 0 ) {
        // Conversion error: capture error code and throw
        const DWORD error = GetLastError();
        throw Exception( {
            Level::Error, error == ERROR_NO_UNICODE_TRANSLATION ?
            "Invalid UTF-8 sequence found in input string."
            :
            "Cannot get result string length when converting from UTF-8 to UTF-16.",
            "LintCombineUtils", 1, 0
        } );
    }

    // Make room in the destination string for the converted bits
    utf16.resize( utf16Length );

    // Do the actual conversion from UTF-8 to UTF-16
    const auto result = MultiByteToWideChar(
        CP_UTF8,       // source string is in UTF-8
        kFlags,        // conversion flags
        utf8.data(),   // source UTF-8 string pointer
        utf8Length,    // length of source UTF-8 string, in chars
        &utf16[0],     // pointer to destination buffer
        utf16Length    // size of destination buffer, in wchar_ts
    );
    if( result == 0 ) {
        // Conversion error: capture error code and throw
        const auto error = GetLastError();
        throw Exception( {
            Level::Error, error == ERROR_NO_UNICODE_TRANSLATION ?
            "Invalid UTF-8 sequence found in input string."
            :
            "Cannot convert from UTF-8 to UTF-16.",
            "LintCombineUtils", 1, 0
        } );
    }

    return utf16;
}

std::string LintCombine::Utf16ToUtf8( const std::wstring & utf16 ) {
    std::string utf8;

    if( utf16.empty() ) {
        return utf8;
    }

    // Safely fail if an invalid UTF-16 character sequence is encountered
    constexpr auto kFlags = WC_ERR_INVALID_CHARS;

    // Safely cast the length of the source UTF-16 string (expressed in wchar_ts)
    // from size_t (returned by std::wstring::length()) to int
    // for the WideCharToMultiByte API.
    // If the size_t value is too big to be stored into an int,
    // throw an exception to prevent conversion errors like huge size_t values
    // converted to *negative* integers.
    if( utf16.length() > static_cast< size_t >( ( std::numeric_limits<int>::max )() ) ) {
        throw Exception( { Level::Error,
                           "Command line too long. Max size is " +
                           std::to_string( ( std::numeric_limits<int>::max )() ),
                           "LintCombineUtils", 1, 0 } );
    }
    const auto utf16Length = static_cast< int >( utf16.length() );

    // Get the length, in chars, of the resulting UTF-8 string
    const auto utf8Length = WideCharToMultiByte(
        CP_UTF8,            // convert to UTF-8
        kFlags,             // conversion flags
        utf16.data(),       // source UTF-16 string
        utf16Length,        // length of source UTF-16 string, in wchar_ts
        nullptr,            // unused - no conversion required in this step
        0,                  // request size of destination buffer, in chars
        nullptr, nullptr    // unused
    );
    if( utf8Length == 0 ) {
        // Conversion error: capture error code and throw
        const auto error = GetLastError();
        throw Exception( {
            Level::Error, error == ERROR_NO_UNICODE_TRANSLATION ?
            "Invalid UTF-16 sequence found in input string."
            :
            "Cannot get result string length when converting from UTF-16 to UTF-8.",
            "LintCombineUtils", 1, 0
        } );
    }

    // Make room in the destination string for the converted bits
    utf8.resize( utf8Length );

    // Do the actual conversion from UTF-16 to UTF-8
    const auto result = WideCharToMultiByte(
        CP_UTF8,            // convert to UTF-8
        kFlags,             // conversion flags
        utf16.data(),       // source UTF-16 string
        utf16Length,        // length of source UTF-16 string, in wchar_ts
        &utf8[0],           // pointer to destination buffer
        utf8Length,         // size of destination buffer, in chars
        nullptr, nullptr    // unused
    );
    if( result == 0 ) {
        // Conversion error: capture error code and throw
        const auto error = GetLastError();
        throw Exception( {
            Level::Error, error == ERROR_NO_UNICODE_TRANSLATION ?
            "Invalid UTF-16 sequence found in input string."
            :
            "Cannot convert from UTF-16 to UTF-8.",
            "LintCombineUtils", 1, 0
        } );
    }

    return utf8;
}

static int getSymCodeInCP437( const wchar_t sym ) {
    static std::unordered_map< wchar_t, int > cp437SymCodesMap = {
        { L'☺', 0x01 }, { L'☻', 0x02 }, { L'♥', 0x03 }, { L'♦', 0x04 }, { L'♣', 0x05 }, { L'♠', 0x06 },
        { L'•', 0x07 }, { L'◘', 0x08 }, { L'○', 0x09 }, { L'◙', 0x0A }, { L'♂', 0x0B }, { L'♀', 0x0C },
        { L'♪', 0x0D }, { L'♫', 0x0E }, { L'☼', 0x0F }, { L'►', 0x10 }, { L'◄', 0x11 }, { L'↕', 0x12 },
        { L'‼', 0x13 }, { L'¶', 0x14 }, { L'§', 0x15 }, { L'▬', 0x16 }, { L'↨', 0x17 }, { L'↑', 0x18 },
        { L'↓', 0x19 }, { L'→', 0x1A }, { L'←', 0x1B }, { L'∟', 0x1C }, { L'↔', 0x1D }, { L'▲', 0x1E },
        { L'▼', 0x1F }, { L'⌂', 0x7F }, { L'Ç', 0x80 }, { L'ü', 0x81 }, { L'é', 0x82 }, { L'â', 0x83 },
        { L'ä', 0x84 }, { L'à', 0x85 }, { L'å', 0x86 }, { L'ç', 0x87 }, { L'ê', 0x88 }, { L'ë', 0x89 },
        { L'è', 0x8A }, { L'ï', 0x8B }, { L'î', 0x8C }, { L'ì', 0x8D }, { L'Ä', 0x8E }, { L'Å', 0x8F },
        { L'É', 0x90 }, { L'æ', 0x91 }, { L'Æ', 0x92 }, { L'ô', 0x93 }, { L'ö', 0x94 }, { L'ò', 0x95 },
        { L'û', 0x96 }, { L'ù', 0x97 }, { L'ÿ', 0x98 }, { L'Ö', 0x99 }, { L'Ü', 0x9A }, { L'¢', 0x9B },
        { L'£', 0x9C }, { L'¥', 0x9D }, { L'₧', 0x9E }, { L'ƒ', 0x9F }, { L'á', 0xA0 }, { L'í', 0xA1 },
        { L'ó', 0xA2 }, { L'ú', 0xA3 }, { L'ñ', 0xA4 }, { L'Ñ', 0xA5 }, { L'ª', 0xA6 }, { L'º', 0xA7 },
        { L'¿', 0xA8 }, { L'⌐', 0xA9 }, { L'¬', 0xAA }, { L'½', 0xAB }, { L'¼', 0xAC }, { L'¡', 0xAD },
        { L'«', 0xAE }, { L'»', 0xAF }, { L'░', 0xB0 }, { L'▒', 0xB1 }, { L'▓', 0xB2 }, { L'│', 0xB3 },
        { L'┤', 0xB4 }, { L'╡', 0xB5 }, { L'╢', 0xB6 }, { L'╖', 0xB7 }, { L'╕', 0xB8 }, { L'╣', 0xB9 },
        { L'║', 0xB1 }, { L'╗', 0xBB }, { L'╝', 0xBC }, { L'╜', 0xBD }, { L'╛', 0xBE }, { L'┐', 0xBF },
        { L'└', 0xC0 }, { L'┴', 0xC1 }, { L'┬', 0xC2 }, { L'├', 0xC3 }, { L'─', 0xC4 }, { L'┼', 0xC5 },
        { L'╞', 0xC6 }, { L'╟', 0xC7 }, { L'╚', 0xC8 }, { L'╔', 0xC9 }, { L'╩', 0xCA }, { L'╦', 0xCB },
        { L'╠', 0xCC }, { L'═', 0xCD }, { L'╬', 0xCE }, { L'╧', 0xCF }, { L'╨', 0xD0 }, { L'╤', 0xD1 },
        { L'╥', 0xD2 }, { L'╙', 0xD3 }, { L'╘', 0xD4 }, { L'╒', 0xD5 }, { L'╓', 0xD6 }, { L'╫', 0xD7 },
        { L'╪', 0xD8 }, { L'┘', 0xD9 }, { L'┌', 0xDA }, { L'█', 0xDB }, { L'▄', 0xDC }, { L'▌', 0xDD },
        { L'▐', 0xDE }, { L'▀', 0xDF }, { L'α', 0xE0 }, { L'ß', 0xE1 }, { L'Γ', 0xE2 }, { L'π', 0xE3 },
        { L'Σ', 0xE4 }, { L'σ', 0xE5 }, { L'µ', 0xE6 }, { L'τ', 0xE7 }, { L'Φ', 0xE8 }, { L'Θ', 0xE9 },
        { L'Ω', 0xEA }, { L'δ', 0xEB }, { L'∞', 0xEC }, { L'φ', 0xED }, { L'ε', 0xEE }, { L'∩', 0xEF },
        { L'≡', 0xF0 }, { L'±', 0xF1 }, { L'≥', 0xF2 }, { L'≤', 0xF3 }, { L'⌠', 0xF4 }, { L'⌡', 0xF5 },
        { L'÷', 0xF6 }, { L'≈', 0xF7 }, { L'°', 0xF8 }, { L'∙', 0xF9 }, { L'·', 0xFA }, { L'√', 0xFB },
        { L'ⁿ', 0xFC }, { L'²', 0xFD }, { L'■', 0xFE }, };
    if( const auto it = cp437SymCodesMap.find( sym );
        it != cp437SymCodesMap.end() ) {
        return it->second;
    }
    return -1;
}

bool LintCombine::doesStringCompletelyExistsInCP437( const std::string & str ) {
    const auto wideStr = Utf8ToUtf16( str );
    for( const auto sym : wideStr ) {
        if( const auto symCode = static_cast< int >( sym );
            ( symCode < 0x20 || symCode > 0x7E ) && getSymCodeInCP437( sym ) == -1 ) {
            return false;
        }
    }
    return true;
}

std::string LintCombine::convertStringEncodingFromUTF8ToCP437( const std::string & strToConvert ) {
    auto wideStrToConvert = Utf8ToUtf16( strToConvert );
    std::string convertedStr;
    convertedStr.reserve( wideStrToConvert.size() );
    for( const auto & sym : wideStrToConvert ) {
        if( const auto symCode = static_cast< int >( sym );
            symCode >= 0x20 && symCode <= 0x7E )
        {
            convertedStr.push_back( static_cast< char >( symCode ) );
            continue;
        }
        if( const auto symCodeInCP437 = getSymCodeInCP437( sym );
            symCodeInCP437 != -1 )
        {
            convertedStr.push_back( static_cast< char >( symCodeInCP437 ) );
            continue;
        }
        convertedStr.push_back( '?' );
    }
    return convertedStr;
}

#endif

LintCombine::StringVector LintCombine::moveCmdLineIntoSTLContainer( const int argc, _char ** argv ) {
    StringVector cmdLine;
    for( auto i = 1; i < argc; ++i ) {
#ifdef WIN32
        cmdLine.emplace_back( Utf16ToUtf8( argv[i] ) );
#else
        cmdLine.emplace_back( argv[i] );
#endif
    }
    normalizeHyphensInCmdLine( cmdLine );
    return cmdLine;
}

void LintCombine::normalizeHyphensInCmdLine( StringVector & cmdLine ) {
    for( auto & cmdLineElem : cmdLine ) {
        if( cmdLineElem.find( "--" ) != 0 && cmdLineElem.find( '-' ) == 0 ) {
            if( cmdLineElem.find( '=' ) != std::string::npos ) {
                // -param=value -> --param=value
                if( cmdLineElem.find( '=' ) != 2 ) {
                    cmdLineElem.insert( 0, "-" );
                }
            }
            // -param value -> --param value
            else if( cmdLineElem.size() > 2 ) {
                cmdLineElem.insert( 0, "-" );
            }
        }
        if( cmdLineElem.find( "--" ) == 0 ) {
            if( cmdLineElem.find( '=' ) != std::string::npos ) {
                // --p=value -> -p=value
                if( cmdLineElem.find( '=' ) == 3 ) {
                    cmdLineElem.erase( cmdLineElem.begin() );
                }
            }
            // --p value -> -p value
            else if( cmdLineElem.size() == 3 ) {
                cmdLineElem.erase( cmdLineElem.begin() );
            }
        }
    }
}

bool LintCombine::isFileCreatable( const std::filesystem::path & filePath ) {
    std::error_code errorCode;
    if( exists( filePath, errorCode ) )
        return true;
    if( errorCode.value() )
        return false;

    create_directory( filePath.parent_path(), errorCode );
    if( errorCode.value() )
        return false;

    std::ofstream file( filePath );
    if( file.fail() )
        return false;
    file.close();
    std::filesystem::remove( filePath );
    return true;
}

void LintCombine::checkIsOptionsValueInit( const std::string & cmdLine,
                                           std::vector< Diagnostic > & diagnostics,
                                           const std::string & optionName, const std::string & option,
                                           const std::string & diagnosticOrigin,
                                           const std::string & textIfOptionDoesNotExists ) {
    if( option.empty() ) {
        const auto optionStartPos = cmdLine.find( std::string( optionName ) );
        if( optionStartPos != std::string::npos ) {
            const auto optionEndInCL = optionStartPos + std::string( optionName ).size();
            diagnostics.emplace_back(
                Level::Warning, "Parameter \"" + optionName + "\" was set but the parameter's "
                "value was not set. The parameter will be ignored.", diagnosticOrigin,
                static_cast< unsigned >( optionStartPos ), static_cast< unsigned >( optionEndInCL ) );
        }
        else if ( !textIfOptionDoesNotExists.empty() && !diagnosticOrigin.empty() ) {
            diagnostics.emplace_back(
                Level::Info, textIfOptionDoesNotExists, diagnosticOrigin, 1, 0 );
        }
    }
}

bool LintCombine::checkCmdLineForEmptiness( const StringVector & cmdLine ) {
    if( cmdLine.empty() ) {
        OutputHelper::printProductInfo();
        OutputHelper::printHelpOption();
    }
    return cmdLine.empty();
}

bool LintCombine::checkCmdLineForHelpOption( const StringVector & cmdLine ) {
    if( std::any_of( cmdLine.cbegin(), cmdLine.cend(),
                     []( const std::string & opt ) { return opt == "--help"; } ) ) {
        OutputHelper::printHelp();
        return true;
    }
    return false;
}

std::vector<LintCombine::Diagnostic>
LintCombine::appendCurrentAndGetAllDiagnostics( std::vector<Diagnostic> & allDiagnostics,
                                                const std::vector<Diagnostic> & currentDiagnostics ) {
    allDiagnostics.insert( allDiagnostics.end(), currentDiagnostics.begin(), currentDiagnostics.end() );
    return allDiagnostics;
}
