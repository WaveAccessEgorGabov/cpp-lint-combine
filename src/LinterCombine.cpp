#include "LintCombineUtils.h"
#include "LinterCombine.h"

#include <fstream>
#include <filesystem>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>

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

void LintCombine::LinterCombine::checkIsRequiredYamlFilesCombinationSpecified() {
    auto atLeastOneLinterYamlPathInit = false;
    std::string linterYamlPath;
    for( const auto & linter : m_linters ) {
        linter->getYamlPath( linterYamlPath );
        if( !linterYamlPath.empty() ) {
            atLeastOneLinterYamlPathInit = true;
        }
    }
    if( m_combinedYamlPath.empty() && atLeastOneLinterYamlPathInit ) {
        m_diagnostics.emplace_back(
            Level::Warning,
            "Some of linters set path to YAML-file, but path to combined YAML-file is not set",
            "LintCombine", 1, 0 );
    }
    if( !m_combinedYamlPath.empty() && !atLeastOneLinterYamlPathInit ) {
        m_diagnostics.emplace_back(
            Level::Error, "No linters YAML-file specified, but path to combined YAML-file is set",
            "LintCombine", 1, 0 );
        throw Exception( diagnostics() );
    }
}

LintCombine::LinterCombine::LinterCombine( const StringVector & cmdLine,
                                           LinterFactoryBase & factory )
    : m_services( factory.getServices() )
{
    if( cmdLine.empty() ) {
        throw Exception( { Level::Error, "Command Line is empty", "LintCombine", 1, 0 } );
    }

    const std::vector< StringVector > lintersCmdLines = splitCmdLineBySubLinters( cmdLine );

    if( lintersCmdLines.empty() ) {
        throw Exception(
            { Level::Error, "No linters specified. Use --sub-linter, see --help.",
              "LintCombine", 1, 0 } );
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
    checkIsRequiredYamlFilesCombinationSpecified();
}

void LintCombine::LinterCombine::callLinter( const std::unique_ptr< IdeBehaviorItf > & ideBehavior ) {
    m_services.getIOService().restart(); // otherwise io_service's work ends
    for( const auto & linter : m_linters ) {
        linter->callLinter( ideBehavior );
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
    if( returnCode == RC_PartialFailure ) {
        m_diagnostics.emplace_back(
            Level::Warning, "Some linters failed while running", "LintCombine", 1, 0 );
    }
    if( returnCode == RC_TotalFailure ) {
        m_diagnostics.emplace_back(
            Level::Error, "All linters failed while running", "LintCombine", 1, 0 );
    }
    return returnCode;
}

LintCombine::CallTotals LintCombine::LinterCombine::updateYaml() {
    // If result YAML-file not exists it's not necessary update linter's YAML-files
    if( m_combinedYamlPath.empty() ) {
        return { /*successNum=*/ 0, /*failNum=*/ 0 };
    }
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
LintCombine::LinterCombine::splitCmdLineBySubLinters( const StringVector & cmdLine ) const {
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
        throw Exception( { Level::Error, ex.what(), "LintCombine", 1, 0 } );
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
        "result-yaml",
        boost::program_options::value< std::string >( &m_combinedYamlPath )->implicit_value( {} ) );
    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( cmdLine ).
               options( optDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & ex ) {
        throw Exception( { Level::Error, ex.what(), "LintCombine", 1, 0 } );
    }

    checkIsOptionsValueInit( boost::algorithm::join( cmdLine, " " ),
                             m_diagnostics, "result-yaml", m_combinedYamlPath,
                             "LintCombine", "Path to combined YAML-file is not set" );
    if( !m_combinedYamlPath.empty() && !isFileCreatable( m_combinedYamlPath ) ) {
        throw Exception( { Level::Error,
                           "Combined YAML-file \"" + m_combinedYamlPath + "\" is not creatable",
                           "LintCombine", 1, 0 } );
    }
}

LintCombine::CallTotals LintCombine::LinterCombine::getYamlPath( std::string & yamlPathOut ) {
    if( m_alreadyTriedToGetYamlPath )
        yamlPathOut = m_combinedYamlPath;
    m_alreadyTriedToGetYamlPath = true;
    if( m_combinedYamlPath.empty() ) {
        yamlPathOut.clear();
        return { /*successNum=*/0, /*failNum=*/0 };
    }
    if( std::filesystem::exists( m_combinedYamlPath ) ) {
        std::ofstream{ m_combinedYamlPath, std::ios_base::trunc };
    }
    CallTotals combineCallTotal;
    for( const auto & linter : m_linters ) {
        std::string lintersYamlPath;
        const auto & linterCallTotal = linter->getYamlPath( lintersYamlPath );
        combineCallTotal += linterCallTotal;
        if( linterCallTotal.failNum ) {
            m_diagnostics.emplace_back(
                Level::Warning, "Linter's YAML-file path \"" + lintersYamlPath + "\" doesn't exist",
                "LintCombine", 1, 0 );
            continue;
        }
        if( lintersYamlPath.empty() ) {
            continue;  // linter didn't write into YAML-file
        }
        try {
            combineYamlFiles( lintersYamlPath );
        }
        catch( std::exception & ex ) {
            m_diagnostics.emplace_back( Level::Error, ex.what(), "LintCombine", 1, 0 );
        }
    }
    if( std::filesystem::exists( m_combinedYamlPath ) ) {
        yamlPathOut = m_combinedYamlPath;
        return combineCallTotal;
    }
    m_diagnostics.emplace_back(
        Level::Error, "Combined YAML-file isn't created", "LintCombine", 1, 0 );
    m_combinedYamlPath.clear();
    yamlPathOut.clear();
    return combineCallTotal;
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
        auto yamlNodeResult = loadYamlNode( m_combinedYamlPath );
        if( !yamlNodeResult.size() )
            return;

        auto yamlNodeToAppend = loadYamlNode( yamlPathToAppend );
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
