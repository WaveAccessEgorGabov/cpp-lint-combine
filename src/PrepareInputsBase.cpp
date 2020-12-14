#include "PrepareInputsBase.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>
#include "LintCombineUtils.h"

LintCombine::StringVector
LintCombine::PrepareInputsBase::transformCmdLine( const StringVector & cmdLineVal ) {
    cmdLine = cmdLineVal;
    m_sourceCmdLine = boost::algorithm::join( cmdLineVal, " " );
    if( !parseSourceCmdLine() )
        return {};
    if( !validateParsedData() )
        return {};
    if( !initLinters() )
        return {};
    initCmdLine();
    return cmdLine;
}

std::vector<LintCombine::Diagnostic>
LintCombine::PrepareInputsBase::diagnostics() const {
    return m_diagnostics;
}

bool LintCombine::PrepareInputsBase::parseSourceCmdLine() {
    boost::program_options::options_description optDesc;
    optDesc.add_options()
        ( "clazy-checks",
          boost::program_options::value< std::string >( &m_clazyChecks )->implicit_value( {} ) )
        ( "clang-extra-args",
          boost::program_options::value< std::string >( &m_clangExtraArgs )->implicit_value( {} ) )
        ( "export-fixes",
          boost::program_options::value< std::string >( &m_pathToCombinedYaml )->implicit_value( {} ) )
        ( "p",
          boost::program_options::value< std::string >( &pathToWorkDir )->implicit_value( {} ) )
        ( "sub-linter",
          boost::program_options::value< StringVector >( &m_lintersNames ) );
    boost::program_options::variables_map vm;
    try {
        const boost::program_options::parsed_options parsed =
            boost::program_options::command_line_parser( cmdLine )
            .style( boost::program_options::command_line_style::default_style |
                    boost::program_options::command_line_style::allow_long_disguise )
            .options( optDesc ).allow_unregistered().run();
        store( parsed, vm );
        notify( vm );
        unrecognizedCollection =
            collect_unrecognized( parsed.options, boost::program_options::include_positional );
    }
    catch( const std::exception & ex ) {
        m_diagnostics.emplace_back( Level::Error, ex.what(), "BasePreparer", 1, 0 );
        return false;
    }
    return true;
}

bool LintCombine::PrepareInputsBase::validateParsedData() {
    checkIsOptionsValueInit( m_sourceCmdLine, m_diagnostics, "export-fixes", m_pathToCombinedYaml,
                             "BasePreparer", "Path to combined YAML-file is not set" );
    checkIsOptionsValueInit(
        m_sourceCmdLine, m_diagnostics, "clang-extra-args", m_clangExtraArgs, "BasePreparer" );
    checkIsOptionsValueInit( m_sourceCmdLine, m_diagnostics, "clazy-checks", m_clazyChecks, "BasePreparer" );
    if( pathToWorkDir.empty() ) {
        m_diagnostics.emplace_back(
            Level::Error, "Path to compilation database is empty.", "BasePreparer", 1, 0 );
        return false;
    }
    return true;
}

bool LintCombine::PrepareInputsBase::initLinters() {
    auto success = true;
    for( const auto & linterName : m_lintersNames ) {
        if( linterName == "clang-tidy" ) {
            lintersOptions.emplace_back(
                std::make_unique< ClangTidyOptions >( pathToWorkDir, !m_pathToCombinedYaml.empty() ) );
        }
        else if( linterName == "clazy" ) {
            std::istringstream iss( m_clangExtraArgs );
            const auto separatedClangExtraArgs =
                StringVector( std::istream_iterator< std::string >{ iss },
                              std::istream_iterator< std::string > {} );
            lintersOptions.emplace_back(
                std::make_unique< ClazyOptions >(
                    pathToWorkDir, !m_pathToCombinedYaml.empty(),
                    m_clazyChecks, separatedClangExtraArgs ) );
        }
        else {
            unsigned searchFrom = 0;
            // skip parameter name and underline value
            if( linterName == "--sub-linter" ) {
                const auto firstPos = static_cast< unsigned >( m_sourceCmdLine.find( linterName ) );
                const auto lastPos = firstPos + static_cast< unsigned >( linterName.size() );
                searchFrom = lastPos;
            }
            const auto firstPos = static_cast< unsigned >( m_sourceCmdLine.find( linterName, searchFrom ) );
            const auto lastPos = firstPos + static_cast< unsigned >( linterName.size() );
            m_diagnostics.emplace_back(
                Level::Error, "Unknown linter name \"" + linterName + "\"", "BasePreparer", firstPos, lastPos );
            success = false;
        }
    }

    if( lintersOptions.empty() ) {
        // Use all linters by default
        std::istringstream iss( m_clangExtraArgs );
        const auto separatedClangExtraArgs =
            StringVector( std::istream_iterator< std::string >{ iss },
                          std::istream_iterator< std::string > {} );
        lintersOptions.emplace_back( std::make_unique< ClangTidyOptions >(
            pathToWorkDir, !m_pathToCombinedYaml.empty() ) );
        lintersOptions.emplace_back(
            std::make_unique< ClazyOptions >(
                pathToWorkDir, !m_pathToCombinedYaml.empty(),
                m_clazyChecks, separatedClangExtraArgs ) );
        m_diagnostics.emplace_back( Level::Info, "All linters are used", "BasePreparer", 1, 0 );
    }
    return success;
}

void LintCombine::PrepareInputsBase::initCmdLine() {
    cmdLine.clear();
    initCommonOptions();
    appendLintersOptionToCmdLine();
}

void LintCombine::PrepareInputsBase::initCommonOptions() {
    if( !m_pathToCombinedYaml.empty() ) {
        cmdLine.emplace_back( "--result-yaml=" + m_pathToCombinedYaml );
    }
}

void LintCombine::PrepareInputsBase::addOptionToLinterByName( const std::string & name,
                                                              const std::string & option ) {
    for( const auto & linterOptions : lintersOptions ) {
        if( linterOptions->name == name ) {
            linterOptions->options.emplace_back( option );
            break;
        }
    }
}

void LintCombine::PrepareInputsBase::addOptionToAllLinters( const std::string & option ) {
    for( const auto & linterOptions : lintersOptions ) {
        linterOptions->options.emplace_back( option );
    }
}

std::string LintCombine::PrepareInputsBase::optionValueToQuotes( const std::string & optionName,
                                                                 const std::string & optionNameWithValue ) {
    return optionName + "\"" + optionNameWithValue.substr( optionName.size(), std::string::npos ) + "\"";
}

void LintCombine::PrepareInputsBase::appendLintersOptionToCmdLine() {
    for( const auto & linterOptions : lintersOptions ) {
        std::copy( linterOptions->options.begin(), linterOptions->options.end(),
                   std::back_inserter( cmdLine ) );
    }
}
