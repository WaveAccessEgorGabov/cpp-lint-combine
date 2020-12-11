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
    if constexpr( BOOST_OS_LINUX ) {
        const auto pathToFileWithMacros = pathToWorkDir + "/macros";

        std::array macrosToDelete = {
                "__has_include(",
        };

        std::array macrosToUndefAfter = {
                "__builtin_va_start", "__has_cpp_attribute",
        };

        std::string result;
        std::ifstream macrosFileSrc( pathToFileWithMacros );
        for ( std::string line; std::getline(macrosFileSrc, line); ) {
            auto macrosToDeleteFound = false;
            for( const auto & macro : macrosToDelete  ) {
                if( line.find( macro ) != std::string::npos ) {
                    macrosToDeleteFound = true;
                }
            }
            if( !macrosToDeleteFound ) {
                result += line + '\n';
            }
        }
        macrosFileSrc.close();

        std::ofstream macrosFileRes( pathToFileWithMacros );
        macrosFileRes << result;
        for( const auto & macro : macrosToUndefAfter )
            macrosFileRes << "#undef " << macro << std::endl;
    }
}
