#include "PrepareInputsVerbatim.h"
#include "LintCombineUtils.h"

#include <boost/program_options.hpp>

LintCombine::StringVector
LintCombine::PrepareInputsVerbatim::transformCmdLine( const StringVector & cmdLineVal ) {
    m_cmdLine = cmdLineVal;
    if( !validateLinters() )
        return {};
    if( !validateCombinedYamlPath() )
        return {};
    return m_cmdLine;
}

std::vector< LintCombine::Diagnostic >
LintCombine::PrepareInputsVerbatim::diagnostics() const {
    return m_diagnostics;
}

bool LintCombine::PrepareInputsVerbatim::validateLinters() {
    m_diagnostics.emplace_back(
        Level::Info, "Options were passed verbatim", "VerbatimPreparer", 1, 0 );
    StringVector lintersNames;
    boost::program_options::options_description optDesc;
    optDesc.add_options()(
        "sub-linter", boost::program_options::value< StringVector >( &lintersNames ) );
    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( m_cmdLine ).
               options( optDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & ex ) {
        m_diagnostics.emplace_back( Level::Error, ex.what(), "VerbatimPreparer", 1, 0 );
        return false;
    }

    if( lintersNames.empty() ) {
        m_diagnostics.emplace_back(
            Level::Error, "No linters specified. Use --sub-linter, see --help.", "VerbatimPreparer", 1, 0 );
        return false;
    }

    auto success = true;
    for( const auto & name : lintersNames ) {
        if( name != "clang-tidy" && name != "clazy" ) {
            m_diagnostics.emplace_back(
                Level::Error, "Unknown linter name: \"" + name + "\"", "VerbatimPreparer", 1, 0 );
            success = false;
        }
    }
    return success;
}

bool LintCombine::PrepareInputsVerbatim::validateCombinedYamlPath() {
    boost::program_options::options_description optDesc;
    std::string combinedYamlPath;
    optDesc.add_options()(
        "result-yaml", boost::program_options::value< std::string >( &combinedYamlPath ) );
    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( m_cmdLine ).
               options( optDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & ex ) {
        m_diagnostics.emplace_back( Level::Error, ex.what(), "VerbatimPreparer", 1, 0 );
        return false;
    }
    if( combinedYamlPath.empty() ) {
        m_diagnostics.emplace_back(
            Level::Info, "Path to combined YAML-file is not set", "VerbatimPreparer", 1, 0 );
        return false;
    }
    if( !isFileCreatable( combinedYamlPath ) ) {
        m_diagnostics.emplace_back(
            Level::Error, "Combined YAML-file \"" + combinedYamlPath + "\" is not creatable",
            "VerbatimPreparer", 1, 0 );
        return false;
    }
    return true;
}
