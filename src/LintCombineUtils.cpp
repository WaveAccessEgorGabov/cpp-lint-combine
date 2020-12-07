#include "LintCombineUtils.h"
#include "LintCombineException.h"

#include <fstream>

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
#endif

LintCombine::StringVector LintCombine::moveCmdLineIntoSTLContainer( const int argc, wchar_t ** argv ) {
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
