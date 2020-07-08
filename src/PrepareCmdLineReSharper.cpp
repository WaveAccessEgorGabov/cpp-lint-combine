#include "PrepareCmdLineReSharper.h"

#include <boost/algorithm/string/replace.hpp>

void LintCombine::PrepareCmdLineReSharper::appendOptionsForSpecificIDE() {
    cmdLine.clear();
    initCommonOptions();
    initUnrecognizedOptions();
    appendLintersOptionToCmdLine();
}

void LintCombine::PrepareCmdLineReSharper::initCommonOptions() {
    if( !m_pathToGeneralYaml.empty() ) {
        cmdLine.emplace_back( "--result-yaml=" + m_pathToGeneralYaml );
    }
}

void LintCombine::PrepareCmdLineReSharper::initUnrecognizedOptions() {
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

void LintCombine::PrepareCmdLineReSharper::addOptionToLinterByName( const std::string & name,
                                                                    const std::string & option ) {
    for( auto & it : m_lintersOptions ) {
        if( it->name == name ) {
            it->options.emplace_back( option );
            break;
        }
    }
}

void LintCombine::PrepareCmdLineReSharper::addOptionToAllLinters( const std::string & option ) {
    for( const auto & it : m_lintersOptions ) {
        it->options.emplace_back( option );
    }
}

std::string
LintCombine::PrepareCmdLineReSharper::optionValueToQuotes( const std::string & optionName,
                                                           const std::string & optionNameWithValue ) {
    return optionName + "\"" +
        optionNameWithValue.substr( optionName.size(), std::string::npos ) + "\"";
}

void
LintCombine::PrepareCmdLineReSharper::appendLintersOptionToCmdLine() {
    for( const auto & it : m_lintersOptions ) {
        std::copy( it->options.begin(), it->options.end(), std::back_inserter( cmdLine ) );
    }
}