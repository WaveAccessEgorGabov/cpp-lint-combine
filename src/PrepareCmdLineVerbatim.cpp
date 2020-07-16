#include "PrepareCmdLineVerbatim.h"

#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

LintCombine::stringVector
LintCombine::PrepareCmdLineVerbatim::transform( const stringVector cmdLineVal ) {
    this->m_cmdLine = cmdLineVal;
    if( validateLinters() ) {
        return stringVector();
    }
    validateGeneralYamlPath();
    return m_cmdLine;
}

std::vector< LintCombine::Diagnostic >
LintCombine::PrepareCmdLineVerbatim::diagnostics() {
    return m_diagnostics;
}

bool LintCombine::PrepareCmdLineVerbatim::validateLinters() {
    m_diagnostics.emplace_back(
        Level::Info, "Options were passed verbatim",
        "VerbatimPreparer", 1, 0 );
    stringVector lintersNames;
    boost::program_options::options_description OptDesc;
    OptDesc.add_options()
        ( "sub-linter",
          boost::program_options::value < stringVector >( &lintersNames ) );

    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( m_cmdLine ).
                options( OptDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & error ) {
        m_diagnostics.emplace_back( Level::Error, error.what(),
                                   "VerbatimPreparer", 1, 0 );
        return true;
    }

    if( lintersNames.empty() ) {
        m_diagnostics.emplace_back( Diagnostic(
            Level::Error,
            "No linters specified. Available linters are: clang-tidy, clazy.",
            "VerbatimPreparer", 1, 0 ) );
        return true;
    }

    auto isErrorOccur = false;
    for( auto & it : lintersNames ) {
        if( it != "clang-tidy" && it != "clazy" ) {
            m_diagnostics.emplace_back(
                Diagnostic( Level::Error,
                "Unknown linter name: \"" + it + "\"",
                "VerbatimPreparer", 1, 0 ) );
            isErrorOccur = true;
        }
    }
    if( isErrorOccur ) {
        return true;
    }
    return false;
}

void LintCombine::PrepareCmdLineVerbatim::validateGeneralYamlPath() {
    boost::program_options::options_description genYamlOptDesc;
    std::string pathToGeneralYaml;
    const std::string pathToGeneralYamlOnError =
        CURRENT_BINARY_DIR "LintersDiagnostics.yaml";
    genYamlOptDesc.add_options()
        ( "result-yaml",
          boost::program_options::value < std::string >( &pathToGeneralYaml )
          ->default_value( pathToGeneralYamlOnError ) );
    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( m_cmdLine ).
                options( genYamlOptDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & error ) {
        m_diagnostics.emplace_back( Diagnostic( Level::Warning, error.what(),
                                    "VerbatimPreparer", 1, 0 ) );
        m_diagnostics.emplace_back( Diagnostic( 
            Level::Info,
            "path to result-yaml changed to " + pathToGeneralYamlOnError,
            "VerbatimPreparer", 1, 0 ) );
        pathToGeneralYaml = pathToGeneralYamlOnError;
    }

    if( pathToGeneralYaml != pathToGeneralYamlOnError ) {
        const auto yamlFilename =
            boost::filesystem::path( pathToGeneralYaml ).filename().string();
        if( !boost::filesystem::portable_name( yamlFilename ) ) {
            m_diagnostics.emplace_back(
                Diagnostic( Level::Warning,
                "Incorrect general yaml filename: \"" + yamlFilename +
                "\"", "VerbatimPreparer", 1, 0 ) );
            m_diagnostics.emplace_back( 
                Diagnostic( Level::Info,
                "path to result-yaml changed to " + pathToGeneralYamlOnError,
                "VerbatimPreparer", 1, 0 ) );
            pathToGeneralYaml = pathToGeneralYamlOnError;
        }
    }

    if( pathToGeneralYaml == pathToGeneralYamlOnError ) {
        m_cmdLine.erase( std::remove_if( std::begin( m_cmdLine ), std::end( m_cmdLine ),
                         [pathToGeneralYaml]( const std::string & str ) -> bool {
                             return str.find( "--result-yaml" ) == 0 ||
                                 str == pathToGeneralYaml;
                         } ), std::end( m_cmdLine ) );
        m_cmdLine.insert( m_cmdLine.begin(), "--result-yaml=" + pathToGeneralYaml );
    }
}
