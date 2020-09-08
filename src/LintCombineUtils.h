#pragma once

#include "LinterItf.h"

#include <filesystem>

namespace LintCombine {
    StringVector moveCmdLineIntoSTLContainer( int argc, char ** argv );

    void fixHyphensInCmdLine( StringVector & cmdLine );

    bool isFileCreatable( const std::filesystem::path & filePath );
}
