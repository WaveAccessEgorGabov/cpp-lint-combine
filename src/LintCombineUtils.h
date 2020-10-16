#pragma once

#include "Diagnostic.h"
#include "LinterItf.h"

#include <filesystem>

namespace LintCombine {
    StringVector moveCmdLineIntoSTLContainer( int argc, char ** argv );

    void normalizeHyphensInCmdLine( StringVector & cmdLine );

    bool isFileCreatable( const std::filesystem::path & filePath );

    void checkIsOptionsValueInit( const std::string & cmdLine,
                                  std::vector < Diagnostic > & diagnostics,
                                  const std::string & optionName, const std::string & option,
                                  const std::string & diagnosticOrigin,
                                  const std::string & textIfOptionDoesNotExists = {} );
}
