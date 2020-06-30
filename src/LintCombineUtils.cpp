#include "LintCombineUtils.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/join.hpp>
#include <iostream>

LintCombine::CommandLinePreparer::CommandLinePreparer( stringVector & commandLine,
                                                        std::string && toolName ) {
    for( const auto & it : commandLine ) {
        if( it == "--verbatim-commands" ) {
            m_output.emplace_back( "Info",
                                    "Option \"--verbatim-commands\" was set so options are passed to combine verbatim.", 0, 0 );
            toolName.clear();
            commandLine.erase(
                std::remove( commandLine.begin(), commandLine.end(),
                "--verbatim-commands" ), commandLine.end() );
            break;
        }
    }

    boost::algorithm::to_lower( toolName );
    if( !commandLine.empty() ) {
        if( toolName == "resharper" ) {
            m_sourceCL = boost::algorithm::join( commandLine, " " );
            prepareCommandLineForReSharper( commandLine );
        }
    }
    else {
        m_output.emplace_back( "Error", "Command line is empty.", 0, 0 );
        m_isErrorWhilePrepareOccur = true;
    }
    printOutput();
}

LintCombine::stringVector LintCombine::CommandLinePreparer::prepareOutput() const {
    LintCombine::stringVector preparedOutput;
    for( const auto & it : m_output ) {
        preparedOutput.emplace_back( it.level + ": " + it.text );
    }
    return preparedOutput;
}

void LintCombine::CommandLinePreparer::printOutput() const {
    for( const auto & it : prepareOutput() ) {
        std::cerr << it << std::endl;
    }
}

std::string LintCombine::CommandLinePreparer::optionValueToQuotes( const std::string & optionName,
                                                                    const std::string & optionNameWithValue ) {
    return optionName + "\"" +
        optionNameWithValue.substr( optionName.size(), std::string::npos ) + "\"";
}

void LintCombine::CommandLinePreparer::addOptionToClangTidy( const std::string & option ) {
    for( auto & it : m_lintersOptions ) {
        if( it->linterName == "clang-tidy" ) {
            it->options.emplace_back( option );
            break;
        }
    }
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
    if( !m_pathToGeneralYaml.empty() ) {
        commandLine.emplace_back( "--result-yaml=" + m_pathToGeneralYaml );
    }
}

void LintCombine::CommandLinePreparer::initUnrecognizedOptions() {
    stringVector filesForAnalize;
    for( auto & unrecognized : m_unrecognizedCollection ) {
        boost::algorithm::replace_all( unrecognized, "\"", "\\\"" );
        std::string strToCompare = "-config=";
        if( unrecognized.find( strToCompare ) == 0 ) {
            addOptionToClangTidy( optionValueToQuotes( strToCompare, unrecognized ) );
            continue;
        }
        strToCompare = "-line-filter=";
        if( unrecognized.find( strToCompare ) == 0 ) {
            addOptionToClangTidy( optionValueToQuotes( strToCompare, unrecognized ) );
            continue;
        }
        strToCompare = "-header-filter=";
        if( unrecognized.find( strToCompare ) == 0 ) {
            addOptionToAllLinters( unrecognized );
            continue;
        }

        // File to analize
        if( unrecognized[0] != '-' && unrecognized[0] != '@' ) {
            filesForAnalize.emplace_back( unrecognized );
            continue;
        }
        addOptionToClangTidy( unrecognized );
    }

    for( const auto & it : filesForAnalize ) {
        addOptionToAllLinters( it );
    }
}

void LintCombine::CommandLinePreparer::initCommandLine( stringVector & commandLine ) {
    // code duplication
    if( std::find_if( std::begin( commandLine ), std::end( commandLine ),
        []( const std::string & str ) -> bool {
            return str.find( "clang-extra-args" ) != std::string::npos; } ) != std::end( commandLine )
                && m_clangExtraArgs.empty() ) {
        m_output.emplace_back( OutputRecord( "Warning", "Parameter "
                               "\"--clang-extra-args\" was set but the parameter's "
                               "value was not set. The parameter will be ignored.", 0, 0 ) );
    }

    if( std::find_if( std::begin( commandLine ), std::end( commandLine ),
        []( const std::string & str ) -> bool {
            return str.find( "clazy-checks" ) != std::string::npos; } ) != std::end( commandLine )
                && m_clazyChecks.empty() ) {
        m_output.emplace_back( OutputRecord( "Warning", "Parameter "
                               "\"--clazy-checks\" was set but the parameter's "
                               "value was not set. The parameter will be ignored.", 0, 0 ) );
    }

    commandLine.clear();
    boost::erase_all( m_clangExtraArgs, "\"" );
    std::istringstream iss( m_clangExtraArgs );
    for( auto & it : m_lintersNames ) {
        if( it != "clang-tidy" && it != "clazy" ) {
            m_output.emplace_back( OutputRecord( "Error", "Unknown linter name: \"" + it + "\"", 0, 0 ) );
            m_isErrorWhilePrepareOccur = true;
            continue;
        }
        if( it == "clang-tidy" ) {
            m_lintersOptions.emplace_back( new ClangTidyOptions( m_pathToWorkDir ) );
        }
        if( it == "clazy" ) {
            m_lintersOptions.emplace_back( new ClazyOptions( m_pathToWorkDir, m_clazyChecks,
                                           stringVector( std::istream_iterator<std::string> { iss },
                                           std::istream_iterator<std::string> {} ) ) );
        }
    }
    if( m_isErrorWhilePrepareOccur ) {
        return;
    }
    if( m_lintersOptions.empty() ) {
        // Use all linters by default
        m_lintersOptions = { new ClangTidyOptions( m_pathToWorkDir ),
           new ClazyOptions( m_pathToWorkDir, m_clazyChecks,
           stringVector( std::istream_iterator<std::string> { iss },
           std::istream_iterator<std::string> {} ) ) };
    }
    initLintCombineOptions( commandLine );
    initUnrecognizedOptions();
    appendLintersOptionToCommandLine( commandLine );
}

void LintCombine::CommandLinePreparer::prepareCommandLineForReSharper( stringVector & commandLine ) {
    // TODO: remove imlicit_value and simple delete incorrect options (clazyChecks, clangExtraArgs)
    for( size_t i = 0; i < commandLine.size(); ++i ) {
        // find named parameter with incorrect sintax
        if( commandLine[i].find( "-" ) == 0
            && commandLine[i].find( "=" ) == commandLine[i].size() - 1 ) {
            // if parameter is clazy-checks or clang-extra-args - Ignore them.
            if( commandLine[i].find( "clazy-checks" ) != std::string::npos
                || commandLine[i].find( "clang-extra-args" ) != std::string::npos ) {
                m_output.emplace_back(
                    OutputRecord( "Warning", "Parameter \"" +
                    commandLine[i] + "\" was set but the parameter's value was not set. "
                    "The parameter will be ignored.", 0, 0 ) );
                commandLine.erase( std::begin( commandLine ) + i );
                --i;
            }
            else {
                m_output.emplace_back(
                    OutputRecord( "Error", "Value of parameter \"" +
                    commandLine[i] + "\" must follow after the equal sign", 0, 0 ) );
                m_isErrorWhilePrepareOccur = true;
                commandLine.erase( std::begin( commandLine ) + i );
                --i;
            }
            continue;
        }

        // --sub-linter=<added to m_lintersName>
        if( commandLine[i].find( "--sub-linter=" ) == 0 ) {
            m_lintersNames.emplace_back( commandLine[i].substr(
                std::string( "--sub-linter=" ).size(), commandLine[i].size() ) );
            commandLine.erase( std::begin( commandLine ) + i );
            --i;
            continue;
        }

        // --sub-linter <added to m_lintersName>
        if( commandLine[i].find( "--sub-linter" ) == 0 ) {
            if( i < commandLine.size() - 1 ) {
                m_lintersNames.emplace_back( commandLine[i + 1] );
            }
            else {
                m_lintersNames.emplace_back( std::string() );
            }
            commandLine.erase( std::begin( commandLine ) + i );
            --i;
        }
    }

    boost::program_options::options_description programOptions;
    programOptions.add_options()
        ( "clazy-checks",
          boost::program_options::value < std::string >( &m_clazyChecks )
          ->implicit_value( std::string() ) )
        ( "clang-extra-args",
          boost::program_options::value < std::string >( &m_clangExtraArgs )
          ->implicit_value( std::string() ) )
        ( "export-fixes",
          boost::program_options::value < std::string >( &m_pathToGeneralYaml ) )
        ( "p",
          boost::program_options::value < std::string >( &m_pathToWorkDir ) );
    boost::program_options::variables_map variablesMap;
    const boost::program_options::parsed_options parsed =
        boost::program_options::command_line_parser( commandLine ).options( programOptions )
        .style( boost::program_options::command_line_style::default_style |
                 boost::program_options::command_line_style::allow_long_disguise )
        .allow_unregistered().run();
    store( parsed, variablesMap );
    notify( variablesMap );
    m_unrecognizedCollection =
        collect_unrecognized( parsed.options, boost::program_options::include_positional );
    // TODO: separate function for data validation here
    if( m_pathToWorkDir.empty() ) {
        m_output.emplace_back( "Error", "Path to compilation database is empty.", 0, 0 );
        m_isErrorWhilePrepareOccur = true;
        return;
    }
    if( m_pathToGeneralYaml.empty() ) {
        m_output.emplace_back( "Error", "Path to yaml-file is empty.", 0, 0 );
        m_isErrorWhilePrepareOccur = true;
        return;
    }
    initCommandLine( commandLine );
}

LintCombine::stringVector LintCombine::moveCommandLineToSTLContainer( const int argc, char ** argv ) {
    stringVector commandLine;
    for( auto i = 1; i < argc; ++i ) {
        commandLine.emplace_back( std::string( argv[i] ) );
    }
    return commandLine;
}
