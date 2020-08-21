#include "PrepareInputsCLion.h"

#include <boost/algorithm/string/replace.hpp>

#include <fstream>
#include <sstream>

void LintCombine::PrepareInputsCLion::appendLintersOptionToCmdLine() {
    stringVector filesForAnalysis;
    for( auto & unrecognized : unrecognizedCollection ) {
        if constexpr( BOOST_OS_WINDOWS ) {
            boost::algorithm::replace_all( unrecognized, "\"", "\\\"" );
        }
        // File to analysis
        if( unrecognized[0] != '-' && unrecognized[0] != '@' ) {
            filesForAnalysis.emplace_back( unrecognized );
            continue;
        }
        addOptionToLinterByName( "clang-tidy", unrecognized );
    }

    for( const auto & it : filesForAnalysis ) {
        addOptionToAllLinters( it );
    }

    PrepareInputsBase::appendLintersOptionToCmdLine();
}

void LintCombine::PrepareInputsCLion::transformFiles() {
    std::ifstream sourceMacrosFile( pathToWorkDir + "/macros" );
    std::ostringstream pBuf;
    pBuf << sourceMacrosFile.rdbuf();
    std::string buf = pBuf.str();
    std::ofstream updatedMacrosFile( pathToWorkDir + "/macros" );

    // In Linux the following macros must be undefined to avoid redefinition,
    // because clazy also defines this macros
    if constexpr( BOOST_OS_LINUX ) {
        updatedMacrosFile << "#undef __clang_version__\n";
        updatedMacrosFile << "#undef __VERSION__\n";
        updatedMacrosFile << "#undef __has_feature\n";
        updatedMacrosFile << "#undef __has_extension\n";
        updatedMacrosFile << "#undef __has_attribute\n";
        updatedMacrosFile << "#undef __has_builtin\n";
    }
    updatedMacrosFile << buf;

    // In Linux clang in not compatible with GCC 8 or higher,
    // so we can't use the following macros
    if constexpr( BOOST_OS_LINUX ) {
        updatedMacrosFile << "#define _GLIBCXX_USE_MAKE_INTEGER_SEQ 1\n";
        updatedMacrosFile << "#undef __builtin_va_start\n";
    }
}