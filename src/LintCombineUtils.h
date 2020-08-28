#pragma once

#include "LinterItf.h"

#include <filesystem>

namespace LintCombine {
    stringVector moveCmdLineIntoSTLContainer( int argc, char ** argv );

    void fixHyphensInCmdLine( stringVector & cmdLine );

    bool isFileCreatable( const std::filesystem::path & filePath );
}




