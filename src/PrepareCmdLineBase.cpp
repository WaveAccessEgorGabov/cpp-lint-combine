#include "PrepareCmdLineBase.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>

// all variable must be emtry before used
LintCombine::stringVector
LintCombine::PrepareCmdLineBase::transform( const stringVector cmdLineVal ) {
    realeaseClassField();
    cmdLine = cmdLineVal;
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
    appendOptionsForSpecificIDE();
    return cmdLine;
}

// TODO: may be remove sort from here, and make diagnostics() const
std::vector<LintCombine::PrepareCmdLineItf::Diagnostic>
LintCombine::PrepareCmdLineBase::diagnostics() {
    std::sort( std::begin( m_diagnostics ),
           std::end( m_diagnostics ),
           []( const Diagnostic & lhs, const Diagnostic & rhs ) {
               return lhs.firstPos < rhs.firstPos;
           } );
    return m_diagnostics;
}

// TODO: validate implicit value
bool LintCombine::PrepareCmdLineBase::parseSourceCmdLine() {
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
            boost::program_options::command_line_parser( cmdLine ).options( programOptions )
            .style( boost::program_options::command_line_style::default_style |
                     boost::program_options::command_line_style::allow_long_disguise )
            .allow_unregistered().run();
        store( parsed, variablesMap );
        notify( variablesMap );
        m_unrecognizedCollection =
            collect_unrecognized( parsed.options, boost::program_options::include_positional );
    }
    catch( const std::exception & ex ) {
        m_diagnostics.emplace_back(
            Diagnostic( ex.what(), Level::Error, 1, 0 ) );
        return true;
    }
    return false;
}

bool LintCombine::PrepareCmdLineBase::validateParsedData() {
    auto isErrorOccur = false;
    if( m_pathToWorkDir.empty() ) {
        m_diagnostics.emplace_back(
            Diagnostic( "Path to compilation database is empty.", Level::Error, 1, 0 ) );
        isErrorOccur = true;
    }
    if( m_pathToGeneralYaml.empty() ) {
        m_diagnostics.emplace_back(
            Diagnostic( "Path to yaml-file is empty.", Level::Error, 1, 0 ) );
        isErrorOccur = true;
    }
    checkIsOptionsValueInit( "clang-extra-args", m_clangExtraArgs );
    checkIsOptionsValueInit( "clazy-checks", m_clazyChecks );

    return isErrorOccur;
}

void LintCombine::PrepareCmdLineBase::checkIsOptionsValueInit( const std::string & optionName,
                                                               const std::string & option ) {
    if( std::find_if( std::begin( cmdLine ), std::end( cmdLine ),
        [&]( const std::string & str ) -> bool {
            return str.find( optionName ) != std::string::npos; } ) != std::end( cmdLine )
                && option.empty() ) {
        const auto warningBeginInCL =
            static_cast< const unsigned int >( m_sourceCL.find( std::string( optionName ) ) );
        const auto warningEndInCL = warningBeginInCL +
            static_cast< const unsigned int >( std::string( optionName ).size() );
        m_diagnostics.emplace_back( Diagnostic( "Parameter "
                                    "\"" + optionName + "\" was set but the parameter's "
                                    "value was not set. The parameter will be ignored.",
                                    Level::Warning, warningBeginInCL, warningEndInCL ) );
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
            m_diagnostics.emplace_back( Diagnostic( "Unknown linter name \""
                                        + it + "\"", Level::Error, 1, 0 ) );
            isErrorOccur = true;
        }
    }
    // TODO: Print info that all linters are used
    if( m_lintersOptions.empty() ) {
        // Use all linters by default
        std::istringstream iss( m_clangExtraArgs );
        m_lintersOptions = { new ClangTidyOptions( m_pathToWorkDir ),
           new ClazyOptions( m_pathToWorkDir, m_clazyChecks,
           stringVector( std::istream_iterator<std::string> { iss },
           std::istream_iterator<std::string> {} ) ) };
    }
    return isErrorOccur;
}

void LintCombine::PrepareCmdLineBase::realeaseClassField() {
    cmdLine.clear();
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
