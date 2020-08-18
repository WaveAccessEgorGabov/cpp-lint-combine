#include "PrepareInputsCLion.h"

#include <boost/algorithm/string/replace.hpp>

#include <fstream>
#include <sstream>

void LintCombine::PrepareInputsCLion::specifyTargetArch() {
    if constexpr( BOOST_OS_WINDOWS ) {
        const std::vector< std::pair< std::string, std::string > > macroTargetPairs =
        {
            // space in both side of macro is done on purpose.
            { " _M_X64 ",   "x86_64-pc-win32" },
            { " _M_IX86 ",  "i386-pc-win32"   },
            { " _M_ARM64 ", "arm64-pc-win32"  },
            { " _M_ARM ",   "arm-pc-win32"    }
        };
        std::ifstream macrosFile( m_pathToWorkDir + "/macros" );
        std::string linterArchExtraArg;
        for( std::string macroDefinition; std::getline( macrosFile, macroDefinition );) {
            for( const auto & [macrosArch, archTriple] : macroTargetPairs ) {
                if( macroDefinition.find( macrosArch ) != std::string::npos ) {
                    if( !linterArchExtraArg.empty() ) {
                        return;
                    }
                    linterArchExtraArg = "--extra-arg-before=\"--target=" + archTriple + "\"";
                }
            }
        }
        if( !linterArchExtraArg.empty() ) {
            addOptionToAllLinters( linterArchExtraArg );
        }
    }
}

void LintCombine::PrepareInputsCLion::appendLintersOptionToCmdLine() {
    stringVector filesForAnalysis;
    for( auto & unrecognized : m_unrecognizedCollection ) {
        if constexpr( BOOST_OS_WINDOWS ) {
            boost::algorithm::replace_all( unrecognized, "\"", "\\\"" );
            specifyTargetArch();
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
    if constexpr( BOOST_OS_LINUX ) {
        std::ifstream macrosFileSrc( m_pathToWorkDir + "/macros" );

        // save source macros
        std::ostringstream sourceMacros;
        sourceMacros << macrosFileSrc.rdbuf();

        std::ofstream macrosFileRes( m_pathToWorkDir + "/macros" );

        // In Linux the following macros must be undefined to avoid redefinition,
        // because clazy also defines this macros
        macrosFileRes << "#undef __clang_version__\n";
        macrosFileRes << "#undef __VERSION__\n";
        macrosFileRes << "#undef __has_feature\n";
        macrosFileRes << "#undef __has_extension\n";
        macrosFileRes << "#undef __has_attribute\n";
        macrosFileRes << "#undef __has_builtin\n";
        macrosFileRes << sourceMacros.str();

        // In Linux clang in not compatible with GCC 8 or higher,
        // so we can't use the following macros
        macrosFileRes << "#define _GLIBCXX_USE_MAKE_INTEGER_SEQ 1\n";
        macrosFileRes << "#undef __builtin_va_start\n";
    }
}