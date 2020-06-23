#include "LintCombineUtils.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iostream>

LintCombine::CommandLinePreparer::CommandLinePreparer( stringVector & commandLine, std::string && toolName ) {
    boost::algorithm::to_lower( toolName );
    if( toolName == "resharper" ) {
        prepareCommandLineForReSharper( commandLine );
    }
}

std::string LintCombine::CommandLinePreparer::optionValueToQuotes( const std::string & optionName,
                                                                   const std::string & optionNameWithValue ) {
    return optionName + "\"" +
           optionNameWithValue.substr( optionName.size(), std::string::npos ) + "\"";
}

void LintCombine::CommandLinePreparer::addOptionToClangTidy( const std::string & option ) {
    m_lintersOptions[ 0 ]->options.emplace_back( option );
}

void LintCombine::CommandLinePreparer::addOptionToAllLinters( const std::string & option ) {
    for( const auto & it : m_lintersOptions ) {
        it->options.emplace_back( option );
    }
}

void LintCombine::CommandLinePreparer::appendLintersOptionToCommandLine( stringVector & commandLine ) const {
    for( const auto & it : m_lintersOptions ) {
        std::copy( it->options.begin(), it->options.end(), std::back_inserter( commandLine ) );
    }
}

void LintCombine::CommandLinePreparer::initLintCombineOptions( stringVector & commandLine ) const {
    commandLine.emplace_back( "--result-yaml=" + m_pathToCommonYaml );
}

void LintCombine::CommandLinePreparer::initUnrecognizedOptions() {
    for( auto & unrecognized : m_unrecognizedCollection ) {
        boost::algorithm::replace_all( unrecognized, "\"", "\\\"" );
        std::string strToCompare = "config=";
        if( unrecognized.find( strToCompare ) != std::string::npos ) {
            unrecognized = optionValueToQuotes( strToCompare, unrecognized );
            addOptionToClangTidy( unrecognized );
            continue;
        }
        strToCompare = "-line-filter=";
        if( unrecognized.find( strToCompare ) == 0 ) {
            unrecognized = optionValueToQuotes( strToCompare, unrecognized );
            addOptionToClangTidy( unrecognized );
            continue;
        }
        strToCompare = "-header-filter=";
        if( unrecognized.find( strToCompare ) == 0 ) {
            addOptionToAllLinters( unrecognized );
            continue;
        }
        strToCompare = "-checks=";
        if( unrecognized.find( strToCompare ) == 0 ) {
            addOptionToClangTidy ( unrecognized.append ( "," + m_clangTidyExtraChecks ) );
            continue;
        }

        strToCompare = "clang-tidy-extra-checks=";
        if( unrecognized.find ( strToCompare ) != std::string::npos ) {
            continue;
        }

        // File to analize
        if( unrecognized[ 0 ] != '@' && unrecognized[ 0 ] != '-' ) {
            addOptionToAllLinters( unrecognized );
            continue;
        }
        addOptionToClangTidy( unrecognized );
    }
}

void LintCombine::CommandLinePreparer::initCommandLine( stringVector & commandLine ) {
    commandLine.clear();
    boost::erase_all ( m_clangExtraArgs, "\"" );
    std::istringstream iss ( m_clangExtraArgs );
    m_lintersOptions = { new ClangTidyOptions ( m_pathToWorkDir ),
                       new ClazyOptions ( m_pathToWorkDir, m_clazyChecks,
                       stringVector ( std::istream_iterator<std::string> { iss },
                       std::istream_iterator<std::string> {} ) ) };
    initLintCombineOptions( commandLine );
    initUnrecognizedOptions();
    appendLintersOptionToCommandLine( commandLine );
}

void LintCombine::CommandLinePreparer::prepareCommandLineForReSharper( stringVector & commandLine ) {
    boost::program_options::options_description programOptions;
    programOptions.add_options()
            ( "verbatim-commands", "pass options verbatim" )
            ( "clazy-checks",
              boost::program_options::value < std::string >( & m_clazyChecks ) )
            ( "clang-extra-args",
              boost::program_options::value < std::string > ( & m_clangExtraArgs ) )
            ( "clang-tidy-extra-checks",
              boost::program_options::value < std::string > ( & m_clangTidyExtraChecks ) )
            ( "export-fixes",
              boost::program_options::value < std::string >( & m_pathToCommonYaml ) )
            ( "p",
              boost::program_options::value < std::string >( & m_pathToWorkDir ) );
    const boost::program_options::parsed_options parsed =
            boost::program_options::command_line_parser( commandLine ).options( programOptions )
                    .style( boost::program_options::command_line_style::default_style |
                            boost::program_options::command_line_style::allow_long_disguise )
                    .allow_unregistered().run();
    boost::program_options::variables_map variablesMap;
    store( parsed, variablesMap );
    notify( variablesMap );
    if( variablesMap.count( "verbatim-commands" ) ) {
        return;
    }
    m_unrecognizedCollection =
            collect_unrecognized( parsed.options, boost::program_options::include_positional );
    initCommandLine( commandLine );
}

LintCombine::stringVector LintCombine::moveCommandLineToSTLContainer( const int argc, char ** argv ) {
    stringVector commandLine;
    for( auto i = 1; i < argc; ++i ) {
        commandLine.emplace_back( std::string( argv[ i ] ) );
    }
    return commandLine;
}
