#include "PrepareCmdLineVerbatim.h"

#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

LintCombine::stringVector LintCombine::PrepareCmdLineVerbatim::transform( const stringVector cmdLineVal ) {
    this->m_cmdLine = cmdLineVal;
    if( parseSourceCmdLine() ) {
        return stringVector();
    }
    if( validateParsedData() ) {
        return stringVector();
    }
    return m_cmdLine;
}

std::vector< LintCombine::Diagnostic > LintCombine::PrepareCmdLineVerbatim::diagnostics() {
    return m_diagnostics;
}

bool LintCombine::PrepareCmdLineVerbatim::parseSourceCmdLine() {
    boost::program_options::options_description OptDesc;
    OptDesc.add_options()
        ( "result-yaml",
          boost::program_options::value < std::string >( &m_pathToGeneralYaml ) )
        ( "sub-linter",
          boost::program_options::value < stringVector >( &m_lintersNames ) );

    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( m_cmdLine ).
                options( OptDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const boost::program_options::error & error ) {
        m_diagnostics.push_back( Diagnostic( error.what(), "VerbatimPreparer",
                                 Level::Error, 1, 0 ) );
        return true;
    }
    catch( const std::exception & error ) {
        m_diagnostics.push_back( Diagnostic( error.what(), "VerbatimPreparer",
                                 Level::Error, 1, 0 ) );
        return true;
    }
    return false;
}

bool LintCombine::PrepareCmdLineVerbatim::validateParsedData() {
    return validateLinters() || validateGeneralYamlPath();
}

bool LintCombine::PrepareCmdLineVerbatim::validateGeneralYamlPath() {
    const std::string pathToGeneralYamlIfError = CURRENT_BINARY_DIR "LintersDiagnostics.yaml";
    if( m_pathToGeneralYaml.empty() ) {
        m_diagnostics.emplace_back(
            Diagnostic( "result-yaml path doesn't set.",
            "VerbatimPreparer", Level::Warning, 1, 0 ) );
        m_pathToGeneralYaml = pathToGeneralYamlIfError;
        m_cmdLine.push_back( "--result-yaml=" + m_pathToGeneralYaml );
        return false;
    }
    const auto yamlFileName = boost::filesystem::path( m_pathToGeneralYaml ).filename().string();
    if( !boost::filesystem::portable_name( yamlFileName ) ) {
        m_diagnostics.emplace_back(
            Diagnostic( "result-yaml path \""
            + m_pathToGeneralYaml + "\" is invalid.",
            "VerbatimPreparer", Level::Warning, 1, 0 ) );
        m_pathToGeneralYaml = pathToGeneralYamlIfError;
        m_cmdLine.erase( std::remove_if( std::begin( m_cmdLine ), std::end( m_cmdLine ),
                         [this]( const std::string & str ) -> bool {
                             return str.find( "--result-yaml" ) == 0 ||
                                 str == m_pathToGeneralYaml;
                         } ), std::end( m_cmdLine ) );
        m_cmdLine.push_back( "--result-yaml=" + m_pathToGeneralYaml );
    }
    return false;
}

bool LintCombine::PrepareCmdLineVerbatim::validateLinters() {
    if( m_lintersNames.empty() ) {
        m_diagnostics.emplace_back(
            Diagnostic( "No linters specified",
            "VerbatimPreparer", Level::Error, 1, 0 ) );
        return true;
    }

    auto isErrorOccur = false;
    for( auto & it : m_lintersNames ) {
        if( it != "clang-tidy" && it != "clazy" ) {
            m_diagnostics.emplace_back(
            Diagnostic( "Unknown linter name: \"" + it + "\"",
                "VerbatimPreparer", Level::Error, 1, 0 ) );
            isErrorOccur = true;
        }
    }
    if( isErrorOccur ) {
        return true;
    }
    return false;
}






