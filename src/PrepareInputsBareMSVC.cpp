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
        if( unrecognized.front() != '-' && unrecognized.front() != '@' ) {
            if( isCalledExplicitly() ) {
                if( !doesStringCompletelyExistsInCP437( unrecognized ) ) {
                    m_diagnostics.emplace_back( Level::Info,
                                                "The path of file to analyze (" + unrecognized + ") "
                                                "contains some Unicode characters. "
                                                "Visual Studio will not display linters checks for this file.",
                                                "BasePreparer", 1, 0 );
                }
            }
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

bool LintCombine::PrepareInputsBareMSVC::isCalledExplicitly() {
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
    return calledExplicitly;
}
