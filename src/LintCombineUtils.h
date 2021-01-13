#pragma once

#include "Diagnostic.h"
#include "LinterItf.h"

#include <filesystem>

#ifdef WIN32
    #define _main wmain
    #define _char wchar_t
#else
    #define _main main
    #define _char char
#endif

namespace LintCombine {
    StringVector moveCmdLineIntoSTLContainer( int argc, _char ** argv );

    void normalizeHyphensInCmdLine( StringVector & cmdLine );

    bool isFileCreatable( const std::filesystem::path & filePath );

    void checkIsOptionsValueInit( const std::string & cmdLine,
                                  std::vector < Diagnostic > & diagnostics,
                                  const std::string & optionName, const std::string & option,
                                  const std::string & diagnosticOrigin,
                                  const std::string & textIfOptionDoesNotExists = {} );

    bool checkCmdLineForEmptiness( const StringVector & cmdLine );

    bool checkCmdLineForHelpOption( const StringVector & cmdLine );

    std::vector< Diagnostic >
    appendCurrentAndGetAllDiagnostics( std::vector< Diagnostic > & allDiagnostics,
                                       const std::vector< Diagnostic > & currentDiagnostics );

    bool doesSymbolExistsInCP437( const wchar_t sym );

#ifdef WIN32
    std::string Utf16ToUtf8( const std::wstring & utf16 );
    std::wstring Utf8ToUtf16( const std::string & utf8 );
#endif
}
