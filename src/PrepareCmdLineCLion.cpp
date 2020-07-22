#include "PrepareCmdLineCLion.h"

#include <fstream>
#include <sstream>

void LintCombine::PrepareCmdLineCLion::initOptionsToSpecificIDE() {
    stringVector filesForAnalize;
    for( auto & unrecognized : m_unrecognizedCollection ) {
        // File to analize
        if( unrecognized[0] != '-' && unrecognized[0] != '@' ) {
            filesForAnalize.emplace_back( unrecognized );
            continue;
        }
        addOptionToLinterByName( "clang-tidy", unrecognized );
    }

    for( const auto & it : filesForAnalize ) {
        addOptionToAllLinters( it );
    }

    std::ifstream sourceMacrosFile( m_pathToWorkDir + "/macros" );
    std::ostringstream pBuf;
    pBuf << sourceMacrosFile.rdbuf();
    std::string buf = pBuf.str();
    // The following macros must be undefined to avoid redefinition,
    // because clazy also defines this macros
    std::ofstream updatedMacrosFile( m_pathToWorkDir + "/macros" );
    updatedMacrosFile << "#undef __clang_version__\n";
    updatedMacrosFile << "#undef __VERSION__\n";
    updatedMacrosFile << "#undef __has_feature\n";
    updatedMacrosFile << "#undef __has_extension\n";
    updatedMacrosFile << "#undef __has_attribute\n";
    updatedMacrosFile << "#undef __has_builtin\n";
    updatedMacrosFile << buf;
    // clang in not compatible with GCC 8 or higher,
    // so we can't use the following macros
    updatedMacrosFile << "#define _GLIBCXX_USE_MAKE_INTEGER_SEQ 1\n";
    updatedMacrosFile << "#undef __builtin_va_start\n";
}
