#include "PrepareCmdLineBase.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>

LintCombine::stringVector
LintCombine::PrepareCmdLineBase::transform( const stringVector cmdLineVal ) {
    realeaseClassField();
    m_cmdLine = cmdLineVal;
    m_sourceCL = boost::algorithm::join( cmdLineVal, " " );
    if( parseSourceCmdLine() ) {
        return stringVector();
    }
    if( validateParsedData() ) {
        return stringVector();
    }
    if( initLinters() ) {
        return stringVector();
    }
    initCmdLine();
    return m_cmdLine;
}

// TODO: may be sort in another place, and make diagnostics() const
std::vector<LintCombine::Diagnostic>
LintCombine::PrepareCmdLineBase::diagnostics() {
    std::sort( std::begin( m_diagnostics ),
           std::end( m_diagnostics ),
           []( const Diagnostic & lhs, const Diagnostic & rhs ) {
               if( lhs.level == rhs.level )
                   return lhs.firstPos < rhs.firstPos;
               return lhs.level < rhs.level;
           } );
    return m_diagnostics;
}

// TODO: validate implicit value
// TODO: delete options witch implicit
bool LintCombine::PrepareCmdLineBase::parseSourceCmdLine() {
    // TODO : parse here --result-yaml. Or make this method virtual.
    boost::program_options::options_description programOptions;
    programOptions.add_options()
        ( "clazy-checks",
          boost::program_options::value < std::string >( &m_clazyChecks )
          ->implicit_value( std::string() ) )
        ( "clang-extra-args",
          boost::program_options::value < std::string >( &m_clangExtraArgs )
          ->implicit_value( std::string() ) )
        ( "export-fixes",
          boost::program_options::value < std::string >( &m_pathToGeneralYaml )
          ->implicit_value( std::string() ) )
        ( "p",
          boost::program_options::value < std::string >( &m_pathToWorkDir )
          ->implicit_value( std::string() ) )
        ( "sub-linter",
          boost::program_options::value < stringVector >( &m_lintersNames ) );
    boost::program_options::variables_map variablesMap;
    try {
        const boost::program_options::parsed_options parsed =
            boost::program_options::command_line_parser( m_cmdLine )
            .style( boost::program_options::command_line_style::default_style |
            boost::program_options::command_line_style::allow_long_disguise )
            .options( programOptions ).allow_unregistered().run();
        store( parsed, variablesMap );
        notify( variablesMap );
        m_unrecognizedCollection =
            collect_unrecognized( parsed.options, boost::program_options::include_positional );
    }
    catch( const std::exception & ex ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Error, ex.what(), "BasePreparer", 1, 0 ) );
        return true;
    }
    return false;
}

bool LintCombine::PrepareCmdLineBase::validateParsedData() {
    auto isErrorOccur = false;
    if( m_pathToWorkDir.empty() ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Error,
            "Path to compilation database is empty.",
            "BasePreparer", 1, 0 ) );
        isErrorOccur = true;
    }
    if( m_pathToGeneralYaml.empty() ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Error,
            "Path to yaml-file is empty.",
            "BasePreparer", 1, 0 ) );
        isErrorOccur = true;
    }
    checkIsOptionsValueInit( "clang-extra-args", m_clangExtraArgs );
    checkIsOptionsValueInit( "clazy-checks", m_clazyChecks );

    return isErrorOccur;
}

void LintCombine::PrepareCmdLineBase::checkIsOptionsValueInit( const std::string & optionName,
                                                               const std::string & option ) {
    if( std::find_if( std::begin( m_cmdLine ), std::end( m_cmdLine ),
        [&]( const std::string & str ) -> bool {
            return str.find( optionName ) != std::string::npos; } ) != std::end( m_cmdLine )
                && option.empty() ) {
        const auto warningBeginInCL =
            static_cast< const unsigned int >( m_sourceCL.find( std::string( optionName ) ) );
        const auto warningEndInCL = warningBeginInCL +
            static_cast< const unsigned int >( std::string( optionName ).size() );
        m_diagnostics.emplace_back(
            Diagnostic( Level::Warning, "Parameter "
            "\"" + optionName + "\" was set but the parameter's "
            "value was not set. The parameter will be ignored.",
            "BasePreparer", warningBeginInCL, warningEndInCL ) );
    }
}

bool LintCombine::PrepareCmdLineBase::initLinters() {
    auto isErrorOccur = false;
    for( auto & it : m_lintersNames ) {
        if( it == "clang-tidy" ) {
            m_lintersOptions.emplace_back( new ClangTidyOptions( m_pathToWorkDir ) );
        }
        else if( it == "clazy" ) {
            std::istringstream iss( m_clangExtraArgs );
            m_lintersOptions.emplace_back(
                new ClazyOptions( m_pathToWorkDir, m_clazyChecks,
                stringVector( std::istream_iterator< std::string >{ iss },
                std::istream_iterator<std::string> {} ) ) );
        }
        else {
            unsigned findFrom = 0;
            if( it == "--sub-linter" ) {
                const auto firstPos = static_cast< unsigned >( m_sourceCL.find( it ) );
                const auto lastPos = firstPos + static_cast< unsigned >( it.size() );
                findFrom = lastPos;
            }
            const auto firstPos = static_cast< unsigned >( m_sourceCL.find( it, findFrom ) );
            const auto lastPos = firstPos + static_cast< unsigned >( it.size() );
            m_diagnostics.emplace_back(
                Diagnostic( Level::Error, "Unknown linter name \""
                + it + "\"", "BasePreparer", firstPos, lastPos ) );
            isErrorOccur = true;
        }
    }

    if( m_lintersOptions.empty() ) {
        // Use all linters by default
        std::istringstream iss( m_clangExtraArgs );
        m_lintersOptions = { new ClangTidyOptions( m_pathToWorkDir ),
           new ClazyOptions( m_pathToWorkDir, m_clazyChecks,
           stringVector( std::istream_iterator<std::string> { iss },
           std::istream_iterator<std::string> {} ) ) };
        m_diagnostics.emplace_back( Diagnostic(
            Level::Info,
            "All linters are used",
            "BasePreparer", 1, 0 ) );
    }
    return isErrorOccur;
}

void LintCombine::PrepareCmdLineBase::initCmdLine() {
    m_cmdLine.clear();
    initCommonOptions();
    initOptionsToSpecificIDE();
    appendLintersOptionToCmdLine();
}

void LintCombine::PrepareCmdLineBase::initCommonOptions() {
    if( !m_pathToGeneralYaml.empty() ) {
        m_cmdLine.emplace_back( "--result-yaml=" + m_pathToGeneralYaml );
    }
}

// TODO: Think about this method
void LintCombine::PrepareCmdLineBase::realeaseClassField() {
    m_cmdLine.clear();
    m_sourceCL.clear();
    m_diagnostics.clear();
    m_pathToGeneralYaml.clear();
    m_pathToWorkDir.clear();
    m_clazyChecks.clear();
    m_clangExtraArgs.clear();
    m_lintersNames.clear();
    m_unrecognizedCollection.clear();
    m_lintersOptions.clear();
}

void LintCombine::PrepareCmdLineBase::addOptionToLinterByName( const std::string & name,
                                                                    const std::string & option ) {
    for( auto & it : m_lintersOptions ) {
        if( it->name == name ) {
            it->options.emplace_back( option );
            break;
        }
    }
}

void LintCombine::PrepareCmdLineBase::addOptionToAllLinters( const std::string & option ) {
    for( const auto & it : m_lintersOptions ) {
        it->options.emplace_back( option );
    }
}

std::string
LintCombine::PrepareCmdLineBase::optionValueToQuotes( const std::string & optionName,
                                                           const std::string & optionNameWithValue ) {
    return optionName + "\"" +
           optionNameWithValue.substr( optionName.size(), std::string::npos ) + "\"";
}

void LintCombine::PrepareCmdLineBase::appendLintersOptionToCmdLine() {
    for( const auto & it : m_lintersOptions ) {
        std::copy( it->options.begin(), it->options.end(), std::back_inserter( m_cmdLine ) );
    }
}
