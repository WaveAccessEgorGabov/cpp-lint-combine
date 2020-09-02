#include "PrepareInputsBase.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>

LintCombine::stringVector
LintCombine::PrepareInputsBase::transformCmdLine( const stringVector & cmdLineVal ) {
    cmdLine = cmdLineVal;
    m_sourceCL = boost::algorithm::join( cmdLineVal, " " );
    if( parseSourceCmdLine() ) {
        return {};
    }
    if( validateParsedData() ) {
        return {};
    }
    if( initLinters() ) {
        return {};
    }
    initCmdLine();
    return cmdLine;
}

std::vector<LintCombine::Diagnostic>
LintCombine::PrepareInputsBase::diagnostics() {
    std::sort( std::begin( diagnosticsList ),
        std::end( diagnosticsList ),
        []( const Diagnostic & lhs, const Diagnostic & rhs ) {
        if( lhs.level == rhs.level )
            return lhs.firstPos < rhs.firstPos;
        return lhs.level < rhs.level;
    } );
    return diagnosticsList;
}

bool LintCombine::PrepareInputsBase::parseSourceCmdLine() {
    boost::program_options::options_description programOptions;
    programOptions.add_options()
        ( "clazy-checks",
        boost::program_options::value< std::string >( &m_clazyChecks )
        ->implicit_value( std::string() ) )
        ( "clang-extra-args",
        boost::program_options::value< std::string >( &m_clangExtraArgs )
        ->implicit_value( std::string() ) )
        ( "export-fixes",
        boost::program_options::value< std::string >( &m_pathToCombinedYaml )
        ->implicit_value( std::string() ) )
        ( "p",
        boost::program_options::value< std::string >( &pathToWorkDir )
        ->implicit_value( std::string() ) )
        ( "sub-linter",
        boost::program_options::value< stringVector >( &m_lintersNames ) );
    boost::program_options::variables_map variablesMap;
    try {
        const boost::program_options::parsed_options parsed =
            boost::program_options::command_line_parser( cmdLine )
            .style( boost::program_options::command_line_style::default_style |
            boost::program_options::command_line_style::allow_long_disguise )
            .options( programOptions ).allow_unregistered().run();
        store( parsed, variablesMap );
        notify( variablesMap );
        unrecognizedCollection =
            collect_unrecognized( parsed.options, boost::program_options::include_positional );
    }
    catch( const std::exception & ex ) {
        diagnosticsList.emplace_back( Level::Error, ex.what(), "BasePreparer", 1, 0 );
        return true;
    }
    return false;
}

bool LintCombine::PrepareInputsBase::validateParsedData() {
    auto isErrorOccur = false;
    if( pathToWorkDir.empty() ) {
        diagnosticsList.emplace_back( Level::Error,
            "Path to compilation database is empty.",
            "BasePreparer", 1, 0 );
        isErrorOccur = true;
    }
    if( m_pathToCombinedYaml.empty() ) {
        diagnosticsList.emplace_back( Level::Error,
            "Path to yaml-file is empty.",
            "BasePreparer", 1, 0 );
        isErrorOccur = true;
    }
    checkIsOptionsValueInit( "clang-extra-args", m_clangExtraArgs );
    checkIsOptionsValueInit( "clazy-checks", m_clazyChecks );

    return isErrorOccur;
}

void LintCombine::PrepareInputsBase::checkIsOptionsValueInit( const std::string & optionName,
    const std::string & option ) {
    if( std::find_if( std::begin( cmdLine ), std::end( cmdLine ),
        [&]( const std::string & str ) -> bool {
        return str.find( optionName ) != std::string::npos; } ) != std::end( cmdLine )
            && option.empty() ) {
        const auto warningBeginInCL =
            static_cast< const unsigned int >( m_sourceCL.find( std::string( optionName ) ) );
        const auto warningEndInCL = warningBeginInCL +
            static_cast< const unsigned int >( std::string( optionName ).size() );
        diagnosticsList.emplace_back( Level::Warning,
            "Parameter \"" + optionName + "\" was set but the parameter's "
            "value was not set. The parameter will be ignored.",
            "BasePreparer", warningBeginInCL, warningEndInCL );
    }
}

bool LintCombine::PrepareInputsBase::initLinters() {
    auto isErrorOccur = false;
    for( auto & it : m_lintersNames ) {
        if( it == "clang-tidy" ) {
            lintersOptions.emplace_back( std::make_unique< ClangTidyOptions >( pathToWorkDir ) );
        }
        else if( it == "clazy" ) {
            std::istringstream iss( m_clangExtraArgs );
            lintersOptions.emplace_back(
                std::make_unique< ClazyOptions >( pathToWorkDir, m_clazyChecks,
                stringVector( std::istream_iterator< std::string >{ iss },
                std::istream_iterator<std::string> {} ) ) );
        }
        else {
            unsigned findFrom = 0;
            // skip parameter name and underline value
            if( it == "--sub-linter" ) {
                const auto firstPos = static_cast< unsigned >( m_sourceCL.find( it ) );
                const auto lastPos = firstPos + static_cast< unsigned >( it.size() );
                findFrom = lastPos;
            }
            const auto firstPos = static_cast< unsigned >( m_sourceCL.find( it, findFrom ) );
            const auto lastPos = firstPos + static_cast< unsigned >( it.size() );
            diagnosticsList.emplace_back( Level::Error,
                "Unknown linter name \"" + it + "\"", "BasePreparer", firstPos, lastPos );
            isErrorOccur = true;
        }
    }

    if( lintersOptions.empty() ) {
        // Use all linters by default
        std::istringstream iss( m_clangExtraArgs );
        lintersOptions.emplace_back( std::make_unique< ClangTidyOptions >( pathToWorkDir ) );
        lintersOptions.emplace_back( std::make_unique< ClazyOptions >( pathToWorkDir, m_clazyChecks,
            stringVector( std::istream_iterator< std::string > { iss },
            std::istream_iterator< std::string > {} ) ) );
        diagnosticsList.emplace_back( Level::Info, "All linters are used", "BasePreparer", 1, 0 );
    }
    return isErrorOccur;
}

void LintCombine::PrepareInputsBase::initCmdLine() {
    cmdLine.clear();
    initCommonOptions();
    specifyTargetArch();
    appendLintersOptionToCmdLine();
}

void LintCombine::PrepareInputsBase::initCommonOptions() {
    if( !m_pathToCombinedYaml.empty() ) {
        cmdLine.emplace_back( "--result-yaml=" + m_pathToCombinedYaml );
    }
}

void LintCombine::PrepareInputsBase::addOptionToLinterByName( const std::string & name,
    const std::string & option ) {
    for( auto & it : lintersOptions ) {
        if( it->name == name ) {
            it->options.emplace_back( option );
            break;
        }
    }
}

void LintCombine::PrepareInputsBase::addOptionToAllLinters( const std::string & option ) {
    for( const auto & it : lintersOptions ) {
        it->options.emplace_back( option );
    }
}

std::string LintCombine::PrepareInputsBase::optionValueToQuotes(
    const std::string & optionName, const std::string & optionNameWithValue ) {
    return optionName + "\"" +
        optionNameWithValue.substr( optionName.size(), std::string::npos ) + "\"";
}

void LintCombine::PrepareInputsBase::appendLintersOptionToCmdLine() {
    for( const auto & it : lintersOptions ) {
        std::copy( it->options.begin(), it->options.end(), std::back_inserter( cmdLine ) );
    }
}
