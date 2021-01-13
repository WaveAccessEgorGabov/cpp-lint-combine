#include "PrepareInputsBareMSVC.h"
#include "LintCombineUtils.h"

#include <boost/algorithm/string/replace.hpp>

void LintCombine::PrepareInputsBareMSVC::appendLintersOptionToCmdLine() {
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

    for( const auto & fileToAnalyze : filesToAnalyze ) {
        addOptionToAllLinters( fileToAnalyze );
    }

    PrepareInputsBase::appendLintersOptionToCmdLine();
}

bool LintCombine::PrepareInputsBareMSVC::validateParsedData() {
    if( const std::string explicitCallDetector = ".ClangTidy";
        pathToWorkDir.compare( pathToWorkDir.size() - explicitCallDetector.size(),
                               explicitCallDetector.size(), explicitCallDetector ) == 0 )
    {
        calledExplicitly = true;
    }
    else if( const std::string implicitCallDetector = "_analysis";
             pathToWorkDir.compare( pathToWorkDir.size() - implicitCallDetector.size(),
                                    implicitCallDetector.size(), implicitCallDetector ) == 0 )
    {
        calledExplicitly = false;
    }
    if( calledExplicitly ) {
        for( const auto sym : Utf8ToUtf16( pathToWorkDir ) ) {
            if( !doesSymbolExistsInCP437( sym ) ) {
                m_diagnostics.emplace_back( Level::Warning,
                                            "The path of file to analyze contains some Unicode characters. "
                                            "Visual Studio will not display linters checks.",
                                            "BasePreparer", 1, 0 );
                break;
            }
        }
    }
    return PrepareInputsBase::validateParsedData();
}
