#include "LintCombineUtils.h"
#include "LinterCombine.h"

#include <fstream>
#include <filesystem>
#include <boost/program_options.hpp>

std::vector<LintCombine::Diagnostic> LintCombine::LinterCombine::diagnostics() const {
    std::vector< Diagnostic > diagnosticsFromAllLinters;
    for( const auto & linter : m_linters ) {
        const auto & linterDiagnostics = linter->diagnostics();
        diagnosticsFromAllLinters.insert( diagnosticsFromAllLinters.end(),
                                          linterDiagnostics.begin(), linterDiagnostics.end() );
    }
    diagnosticsFromAllLinters.insert( diagnosticsFromAllLinters.end(),
                                      m_diagnostics.begin(), m_diagnostics.end() );
    return diagnosticsFromAllLinters;
}

LintCombine::LinterCombine::LinterCombine( const StringVector & cmdLine,
                                           LinterFactoryBase & factory )
    : m_services( factory.getServices() )
{
    if( cmdLine.empty() ) {
        m_diagnostics.emplace_back(
            Level::Error, "Command Line is empty", "LintCombine", 1, 0 );
        throw Exception( m_diagnostics );
    }

    const std::vector< StringVector > lintersCmdLines = splitCmdLineBySubLinters( cmdLine );

    if( lintersCmdLines.empty() ) {
        m_diagnostics.emplace_back(
            Level::Error, "No linters specified. Use --sub-linter, see --help.", "LintCombine", 1, 0 );
        throw Exception( m_diagnostics );
    }

    for( const auto & linterCmdLine : lintersCmdLines ) {
        auto linter = factory.createLinter( linterCmdLine );
        if( !linter ) {
            throw Exception(
                { Level::Error, "Unknown linter name: \"" + *linterCmdLine.begin() + "\"",
                  "LintCombine", 1, 0 } );
        }
        m_linters.emplace_back( std::move( linter ) );
    }
    initCombinedYamlPath( cmdLine );
}

void LintCombine::LinterCombine::callLinter() {
    m_services.getIOService().restart(); // otherwise io_service's work ends
    for( const auto & linter : m_linters ) {
        linter->callLinter();
    }
}

int LintCombine::LinterCombine::waitLinter() {
    if( m_linters.empty() )
        return 0;
    int returnCode = 1;
    m_services.getIOService().run();
    for( const auto & subLinterIt : m_linters ) {
        subLinterIt->waitLinter() == 0 ? ( returnCode &= ~1 ) : ( returnCode |= 2 );
    }
    if( returnCode == SomeLintersFailed ) {
        m_diagnostics.emplace_back(
            Level::Warning, "Some linters failed while running", "LintCombine", 1, 0 );
    }
    if( returnCode == AllLintersFailed ) {
        m_diagnostics.emplace_back(
            Level::Error, "All linters failed while running", "LintCombine", 1, 0 );
    }
    return returnCode;
}

LintCombine::CallTotals LintCombine::LinterCombine::updateYaml() {
    CallTotals callTotals;
    for( const auto & subLinterIt : m_linters ) {
        callTotals += subLinterIt->updateYaml();
    }
    if( callTotals.failNum ) {
        m_diagnostics.emplace_back(
            Level::Warning, "Updating " + std::to_string( callTotals.failNum ) + " YAML-files failed",
            "LintCombine", 1, 0 );
    }
    return callTotals;
}

std::shared_ptr< LintCombine::LinterItf >
LintCombine::LinterCombine::linterAt( const size_t pos ) const {
    if( pos >= m_linters.size() )
        throw std::out_of_range( "index out of bounds" );
    return m_linters[pos];
}

size_t LintCombine::LinterCombine::numLinters() const noexcept {
    return m_linters.size();
}

std::vector< LintCombine::StringVector >
LintCombine::LinterCombine::splitCmdLineBySubLinters( const StringVector & cmdLine ) {
    StringVector lintersNames;
    boost::program_options::options_description optDesc;
    optDesc.add_options()(
        "sub-linter", boost::program_options::value< StringVector >( &lintersNames ) );
    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( cmdLine ).
               options( optDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & ex ) {
        m_diagnostics.emplace_back( Level::Error, ex.what(), "LintCombine", 1, 0 );
        throw Exception( m_diagnostics );
    }

    StringVector currentSubLinter;
    std::vector< StringVector > subLintersVec;
    for( size_t i = 0, linterNum = 0; i < cmdLine.size(); ++i ) {
        if( linterNum != lintersNames.size() && cmdLine[i] ==
            "--sub-linter=" + lintersNames[linterNum] ) {
            if( linterNum != 0 ) {
                subLintersVec.emplace_back( currentSubLinter );
                currentSubLinter.clear();
            }
            currentSubLinter.emplace_back( lintersNames[linterNum] );
            if( i == cmdLine.size() - 1 ) {
                subLintersVec.emplace_back( currentSubLinter );
            }
            ++linterNum;
        }
        else if( !currentSubLinter.empty() ) {
            currentSubLinter.emplace_back( cmdLine[i] );
            if( i == cmdLine.size() - 1 ) {
                subLintersVec.emplace_back( currentSubLinter );
            }
        }
    }
    return subLintersVec;
}

void LintCombine::LinterCombine::initCombinedYamlPath( const StringVector & cmdLine ) {
    boost::program_options::options_description optDesc;
    optDesc.add_options()(
        "result-yaml", boost::program_options::value< std::string >( &m_combinedYamlPath ) );
    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( cmdLine ).
               options( optDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & ex ) {
        m_diagnostics.emplace_back( Level::Error, ex.what(), "LintCombine", 1, 0 );
        throw Exception( m_diagnostics );
    }

    if( m_combinedYamlPath.empty() ) {
        m_diagnostics.emplace_back(
            Level::Error, "Path to combined YAML-file is not set", "LintCombine", 1, 0 );
        throw Exception( m_diagnostics );
    }

    if( !isFileCreatable( m_combinedYamlPath ) ) {
        m_diagnostics.emplace_back(
            Level::Error, "Combined YAML-file \"" + m_combinedYamlPath + "\" is not creatable",
            "LintCombine", 1, 0 );
        throw Exception( m_diagnostics );
    }
}

std::string LintCombine::LinterCombine::getYamlPath() {
    if( m_alreadyTriedToGetYamlPath )
        return m_combinedYamlPath;
    m_alreadyTriedToGetYamlPath = true;
    if( !m_combinedYamlPath.empty() ) {
        if( std::filesystem::exists( m_combinedYamlPath ) ) {
            std::ofstream{ m_combinedYamlPath, std::ios_base::trunc };
        }
        for( const auto & linter : m_linters ) {
            const auto & lintersYamlPath = linter->getYamlPath();
            if( lintersYamlPath.empty() ) {
                m_diagnostics.emplace_back(
                    Level::Warning, "Linter's YAML file path value is empty", "LintCombine", 1, 0 );
                continue;
            }
            if( !std::filesystem::exists( lintersYamlPath ) ) {
                m_diagnostics.emplace_back(
                    Level::Warning, "Linter's YAML-file path \"" + lintersYamlPath + "\" doesn't exist",
                    "LintCombine", 1, 0 );
                continue;
            }
            try {
                combineYamlFiles( lintersYamlPath );
            }
            catch( std::exception & ex ) {
                m_diagnostics.emplace_back( Level::Error, ex.what(), "LintCombine", 1, 0 );
            }
        }
    }
    else {
        m_diagnostics.emplace_back(
            Level::Error, "Combined YAML-file path value is empty", "LintCombine", 1, 0 );
    }
    if( std::filesystem::exists( m_combinedYamlPath ) ) {
        return m_combinedYamlPath;
    }
    m_diagnostics.emplace_back(
        Level::Error, "Combined YAML-file isn't created", "LintCombine", 1, 0 );
    m_combinedYamlPath.clear();
    return m_combinedYamlPath;
}

void LintCombine::LinterCombine::combineYamlFiles( const std::string & yamlPathToAppend ) {
    if( !std::filesystem::exists( m_combinedYamlPath ) ) {
        try {
            std::filesystem::copy( yamlPathToAppend, m_combinedYamlPath );
        }
        catch( const std::exception & ex ) {
            m_diagnostics.emplace_back( Level::Error, ex.what(), "LintCombine", 1, 0 );
        }
    }
    else if( std::filesystem::is_empty( m_combinedYamlPath ) ) {
        std::ifstream infile( yamlPathToAppend );
        std::ofstream outfile( m_combinedYamlPath );
        outfile << infile.rdbuf();
    }
    else {
        YAML::Node yamlNodeResult = loadYamlNode( m_combinedYamlPath );
        if( !yamlNodeResult.size() )
            return;

        YAML::Node yamlNodeToAppend = loadYamlNode( yamlPathToAppend );
        if( !yamlNodeToAppend.size() )
            return;

        for( const auto & diagnostic : yamlNodeToAppend["Diagnostics"] ) {
            yamlNodeResult["Diagnostics"].push_back( diagnostic );
        }

        try {
            std::ofstream mergedYamlOutputFile( m_combinedYamlPath );
            mergedYamlOutputFile << yamlNodeResult;
        }
        catch( const std::exception & ex ) {
            m_diagnostics.emplace_back( Level::Error, ex.what(), "LintCombine", 1, 0 );
        }
    }
}

YAML::Node LintCombine::LinterCombine::loadYamlNode( const std::string & pathToYaml ) {
    const std::ifstream filePathToYaml( pathToYaml );
    if( !std::filesystem::exists( pathToYaml ) ) {
        m_diagnostics.emplace_back(
            Level::Warning, "YAML-file path \"" + pathToYaml + "\" doesn't exist", "LinterBase", 1, 0 );
        return {};
    }
    if( filePathToYaml.fail() ) {
        m_diagnostics.emplace_back(
            Level::Warning,
            "An error occurred while opening \"" + pathToYaml + "\" for reading", "LinterBase", 1, 0 );
        return {};
    }
    return YAML::LoadFile( pathToYaml );
}
