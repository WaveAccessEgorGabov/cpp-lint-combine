#include "PrepareInputsCLion.h"

#include <boost/algorithm/string/replace.hpp>

#include <fstream>
#include <sstream>

void LintCombine::PrepareInputsCLion::specifyTargetArch() {
    if constexpr( BOOST_OS_WINDOWS ) {
        const std::pair< std::string, std::string > macroTargetPairs[] =
        {
            // Note spaces on both sides of macro names
            { " _M_X64 ",   "x86_64-pc-win32" },
            { " _M_IX86 ",  "i386-pc-win32"   },
            { " _M_ARM64 ", "arm64-pc-win32"  },
            { " _M_ARM ",   "arm-pc-win32"    },
        };
        std::ifstream macrosFile( pathToWorkDir + "/macros" );
        std::string linterArchExtraArg;
        for( std::string macroDefinition; std::getline( macrosFile, macroDefinition );) {
            for( const auto & [archMacro, archTriple] : macroTargetPairs ) {
                if( macroDefinition.find( archMacro ) != std::string::npos ) {
                    if( !linterArchExtraArg.empty() ) {
                        return; // several different target architectures specified
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
    StringVector filesToAnalyze;
    for( auto & unrecognized : unrecognizedCollection ) {
        if constexpr( BOOST_OS_WINDOWS ) {
            boost::algorithm::replace_all( unrecognized, "\"", "\\\"" );
        }
        // File to analyze
        if( unrecognized[0] != '-' && unrecognized[0] != '@' ) {
            filesToAnalyze.emplace_back( unrecognized );
            continue;
        }
        addOptionToLinterByName( "clang-tidy", unrecognized );
    }
    specifyTargetArch();

    for( const auto & fileToAnalyze : filesToAnalyze ) {
        addOptionToAllLinters( fileToAnalyze );
    }

    PrepareInputsBase::appendLintersOptionToCmdLine();
}

void LintCombine::PrepareInputsCLion::transformFiles() {
    const auto pathToFileWithMacros = pathToWorkDir + "/macros";
    if constexpr( BOOST_OS_WINDOWS ) {
        std::ofstream macrosFileRes( pathToFileWithMacros, std::ios_base::app );
        std::string macrosToUndefAfter[] = {
            "__has_cpp_attribute",
        };

        for( const auto & macro : macrosToUndefAfter ) {
            macrosFileRes << "#undef " << macro << std::endl;
        }
    }

    if constexpr( BOOST_OS_LINUX ) {
        std::ifstream macrosFileSrc( pathToFileWithMacros );

        // save source macros
        std::ostringstream sourceMacros;
        sourceMacros << macrosFileSrc.rdbuf();
        macrosFileSrc.close();

        std::ofstream macrosFileRes( pathToFileWithMacros );

        // TODO: Do we really need macros above?
        // In Linux the following macros must be undefined to avoid redefinition,
        // because clazy also defines these macros
        std::string macrosToUndefBefore[] = {
            "__clang_version__", "__VERSION__", "__has_feature",
            "__has_extension", "__has_attribute", "__has_builtin",
        };

        // TODO: Do we really need macros above?
        // In Linux clang in not compatible with GCC 8 or higher,
        // so we can't use the following macros
        std::string macrosToUndefAfter[] = {
            "__builtin_va_start"
        };

        for( const auto & macro : macrosToUndefBefore )
            macrosFileRes << "#undef " << macro << std::endl;

        macrosFileRes << sourceMacros.str();

        for( const auto & macro : macrosToUndefAfter )
            macrosFileRes << "#undef " << macro << std::endl;
    }
}