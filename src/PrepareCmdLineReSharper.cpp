#include "PrepareCmdLineReSharper.h"

#include <boost/algorithm/string/replace.hpp>

void LintCombine::PrepareCmdLineReSharper::initOptionsToSpecificIDE() {
    stringVector filesForAnalize;
    for( auto & unrecognized : m_unrecognizedCollection ) {
        boost::algorithm::replace_all( unrecognized, "\"", "\\\"" );
        std::string strToCompare = "--config=";
        if( unrecognized.find( strToCompare ) == 0 ) {
            addOptionToLinterByName( "clang-tidy",
                                     optionValueToQuotes( strToCompare, unrecognized ) );
            continue;
        }
        strToCompare = "--line-filter=";
        if( unrecognized.find( strToCompare ) == 0 ) {
            addOptionToLinterByName( "clang-tidy",
                                     optionValueToQuotes( strToCompare, unrecognized ) );
            continue;
        }
        strToCompare = "--header-filter=";
        if( unrecognized.find( strToCompare ) == 0 ) {
            addOptionToAllLinters( unrecognized );
            continue;
        }

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
}
