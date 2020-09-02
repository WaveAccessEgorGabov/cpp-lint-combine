#include "PrepareInputsVerbatim.h"
#include "LintCombineUtils.h"

#include <boost/program_options.hpp>

LintCombine::stringVector
LintCombine::PrepareInputsVerbatim::transformCmdLine( const stringVector & cmdLineVal ) {
    m_cmdLine = cmdLineVal;
    if( validateLinters() ) {
        return {};
    }
    if( validateCombinedYamlPath() ) {
        return {};
    }
    return m_cmdLine;
}

std::vector< LintCombine::Diagnostic >
LintCombine::PrepareInputsVerbatim::diagnostics() {
    return m_diagnostics;
}

bool LintCombine::PrepareInputsVerbatim::validateLinters() {
    m_diagnostics.emplace_back( Level::Info,
        "Options were passed verbatim", "VerbatimPreparer", 1, 0 );
    stringVector lintersNames;
    boost::program_options::options_description optDesc;
    optDesc.add_options()
        ( "sub-linter",
        boost::program_options::value< stringVector >( &lintersNames ) );

    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( m_cmdLine ).
            options( optDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & error ) {
        m_diagnostics.emplace_back(
            Level::Error, error.what(), "VerbatimPreparer", 1, 0 );
        return true;
    }

    if( lintersNames.empty() ) {
        m_diagnostics.emplace_back( Level::Error,
            "No linters specified. Supported linters are: clang-tidy, clazy.",
            "VerbatimPreparer", 1, 0 );
        return true;
    }

    auto errorOccur = false;
    for( auto & it : lintersNames ) {
        if( it != "clang-tidy" && it != "clazy" ) {
            m_diagnostics.emplace_back( Level::Error,
                "Unknown linter name: \"" + it + "\"",
                "VerbatimPreparer", 1, 0 );
            errorOccur = true;
        }
    }
    if( errorOccur ) {
        return true;
    }
    return false;
}

bool LintCombine::PrepareInputsVerbatim::validateCombinedYamlPath() {
    boost::program_options::options_description optDesc;
    std::string combinedYAMLPath;
    optDesc.add_options()
        ( "result-yaml",
        boost::program_options::value< std::string >( &combinedYAMLPath ) );
    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( m_cmdLine ).
            options( optDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & error ) {
        m_diagnostics.emplace_back(
            Level::Error, error.what(), "VerbatimPreparer", 1, 0 );
        return true;
    }
    if( combinedYAMLPath.empty() ) {
        m_diagnostics.emplace_back( Level::Error,
            "Path to combined YAML-file is not set", "VerbatimPreparer", 1, 0 );
        return true;
    }
    if( !isFileCreatable( combinedYAMLPath ) ) {
        m_diagnostics.emplace_back( Level::Error,
            "Combined YAML-file \"" + combinedYAMLPath + "\" is not creatable",
            "VerbatimPreparer", 1, 0 );
        return true;
    }
    return false;
}
