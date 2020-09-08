#include "PrepareInputsReSharper.h"

#include <boost/algorithm/string/replace.hpp>

void LintCombine::PrepareInputsReSharper::appendLintersOptionToCmdLine() {
    StringVector filesToAnalyze;
    for( auto & unrecognized : unrecognizedCollection ) {
        boost::algorithm::replace_all( unrecognized, "\"", "\\\"" );
        std::string strToCompare = "--config=";
        if( unrecognized.find( strToCompare ) == 0 ) {
            addOptionToLinterByName(
                "clang-tidy", optionValueToQuotes( strToCompare, unrecognized ) );
            continue;
        }
        strToCompare = "--line-filter=";
        if( unrecognized.find( strToCompare ) == 0 ) {
            addOptionToLinterByName(
                "clang-tidy", optionValueToQuotes( strToCompare, unrecognized ) );
            continue;
        }
        strToCompare = "--header-filter=";
        if( unrecognized.find( strToCompare ) == 0 ) {
            addOptionToAllLinters( unrecognized );
            continue;
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
