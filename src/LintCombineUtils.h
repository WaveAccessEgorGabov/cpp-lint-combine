#pragma once

#include "LinterItf.h"

#include <filesystem>

namespace LintCombine {
    StringVector moveCmdLineIntoSTLContainer( int argc, char ** argv );

    void normalizeHyphensInCmdLine( StringVector & cmdLine );

    bool isFileCreatable( const std::filesystem::path & filePath );
}
