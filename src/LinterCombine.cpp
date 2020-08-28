#include "LintCombineUtils.h"
#include "LinterCombine.h"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

std::vector<LintCombine::Diagnostic> LintCombine::LinterCombine::diagnostics() {
    std::vector< Diagnostic > allDiagnostics;
    for( const auto & subLinterIt : m_linters ) {
        const auto & linterDiagnostics = subLinterIt->diagnostics();
        allDiagnostics.insert( allDiagnostics.end(),
                               linterDiagnostics.begin(), linterDiagnostics.end() );
    }
    allDiagnostics.insert( allDiagnostics.end(),
                           m_diagnostics.begin(), m_diagnostics.end() );
    return allDiagnostics;
}

LintCombine::LinterCombine::LinterCombine( const stringVector & cmdLine,
                                           LinterFactoryBase & factory )
    : m_services( factory.getServices() ) {

    if( cmdLine.empty() ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Error,
            "Command Line is empty", "Combine", 1, 0 ) );
        throw Exception( m_diagnostics );
    }

    const std::vector < stringVector > subLintersCmdLine =
        splitCommandLineBySubLinters( cmdLine );

    if( subLintersCmdLine.empty() ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Error,
            "No one linter parsed",
            "Combine", 1, 0 ) );
        throw Exception( m_diagnostics );
    }

    for( const auto & it : subLintersCmdLine ) {
        const auto subLinter = factory.createLinter( it );
        if( subLinter == nullptr ) {
            m_diagnostics.emplace_back(
                Diagnostic( Level::Error,
                "Unknown linter name: \"" + *it.begin() + "\"",
                "Combine", 1, 0 ) );
            throw Exception( m_diagnostics );
        }
        m_linters.emplace_back( subLinter );
    }

    validateGeneralYamlPath( cmdLine );
}

void LintCombine::LinterCombine::callLinter() {
    // to continue work's of io_service
    m_services.getIOService().restart();
    for( const auto & subLinterIt : m_linters ) {
        subLinterIt->callLinter();
    }
}

int LintCombine::LinterCombine::waitLinter() {
    if( m_linters.empty() ) {
        return 0;
    }
    int returnCode = 1;
    m_services.getIOService().run();
    for( const auto & subLinterIt : m_linters ) {
        subLinterIt->waitLinter() == 0 ? ( returnCode &= ~1 ) :
            ( returnCode |= 2 );
    }
    if( returnCode == 2 ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Warning,
            "Some linters failed while running",
            "Combine", 1, 0 ) );
    }
    if( returnCode == 3 ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Error,
            "All linters failed while running",
            "Combine", 1, 0 ) );
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
            Diagnostic( Level::Error,
            "Updating " + std::to_string( callTotals.failNum )
            + " YAML files failed", "Combine", 1, 0 ) );
    }
    return callTotals;
}

std::shared_ptr < LintCombine::LinterItf > LintCombine::LinterCombine::linterAt( const size_t pos ) const {
    if( pos >= m_linters.size() )
        throw std::out_of_range( "index out of bounds" );
    return m_linters[pos];
}

size_t LintCombine::LinterCombine::numLinters() const noexcept {
    return m_linters.size();
}

std::vector < LintCombine::stringVector >
LintCombine::LinterCombine::splitCommandLineBySubLinters( const stringVector & commandLine ) {
    stringVector lintersNames;
    boost::program_options::options_description linterOptDesc;
    linterOptDesc.add_options()
        ( "sub-linter",
          boost::program_options::value < stringVector >( &lintersNames ) );
    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( commandLine ).
               options( linterOptDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & error ) {
        m_diagnostics.emplace_back( Diagnostic( Level::Error, error.what(),
                                    "Combine", 1, 0 ) );
        throw Exception( m_diagnostics );
    }

    stringVector currentSubLinter;
    std::vector < stringVector > subLintersVec;
    for( size_t i = 0, linterNum = 0; i < commandLine.size(); ++i ) {
        if( linterNum != lintersNames.size() && commandLine[i] ==
            "--sub-linter=" + lintersNames[linterNum] ) {
            if( linterNum != 0 ) {
                subLintersVec.emplace_back( currentSubLinter );
                currentSubLinter.clear();
            }
            currentSubLinter.emplace_back( lintersNames[linterNum] );
            if( i == commandLine.size() - 1 ) {
                subLintersVec.emplace_back( currentSubLinter );
            }
            ++linterNum;
        }
        else if( !currentSubLinter.empty() ) {
            currentSubLinter.emplace_back( commandLine[i] );
            if( i == commandLine.size() - 1 ) {
                subLintersVec.emplace_back( currentSubLinter );
            }
        }
    }
    return subLintersVec;
}

void LintCombine::LinterCombine::validateGeneralYamlPath( const stringVector & cmdLine ) {
    boost::program_options::options_description optDesc;
    optDesc.add_options()
        ( "result-yaml",
          boost::program_options::value < std::string >( &m_generalYAMLPath ) );
    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( cmdLine ).
               options( optDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & error ) {
        m_diagnostics.emplace_back( Diagnostic( Level::Error, error.what(),
                                    "Combine", 1, 0 ) );
        throw Exception( m_diagnostics );
    }

    if( m_generalYAMLPath.empty() ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Error, "Path to general YAML-file is not set",
            "Combine", 1, 0 ) );
        throw Exception( m_diagnostics );
    }

    if( !isFileCreatable( m_generalYAMLPath ) ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Error, "General YAML-file \"" +
            m_generalYAMLPath + "\" is not creatable",
            "Combine", 1, 0 ) );
        throw Exception( m_diagnostics );
    }
}

const std::string & LintCombine::LinterCombine::getYamlPath() {
    if( !m_generalYAMLPath.empty() ) {
        for( const auto & subLinterIt : m_linters ) {
            if( subLinterIt->getYamlPath().empty() ) {
                m_diagnostics.emplace_back(
                    Diagnostic( Level::Warning,
                    "Linter's YAML file path value is empty",
                    "Combine", 1, 0 ) );
                continue;
            }
            if( !boost::filesystem::exists( subLinterIt->getYamlPath() ) ) {
                m_diagnostics.emplace_back(
                    Diagnostic( Level::Warning,
                    "Linter's YAML file path \"" + subLinterIt->getYamlPath()
                    + "\" doesn't exist", "Combine", 1, 0 ) );
                continue;
            }
            try {
                mergeYaml( subLinterIt->getYamlPath() );
            }
            catch( std::exception & error ) {
                m_diagnostics.emplace_back(
                    Diagnostic( Level::Error, error.what(),
                    "Combine", 1, 0 ) );
            }
        }
    }
    else {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Error,
            "General YAML file path value is empty",
            "Combine", 1, 0 ) );
    }
    if( boost::filesystem::exists( m_generalYAMLPath ) ) {
        return m_generalYAMLPath;
    }
    m_diagnostics.emplace_back(
        Diagnostic( Level::Error,
        "General YAML file isn't created",
        "Combine", 1, 0 ) );
    m_generalYAMLPath.clear();
    return m_generalYAMLPath;
}

void LintCombine::LinterCombine::mergeYaml( const std::string & yamlPathToMerge ) {
    if( !boost::filesystem::exists( m_generalYAMLPath ) ) {
        try {
            boost::filesystem::copy( yamlPathToMerge, m_generalYAMLPath );
        }
        catch( std::exception & error ) {
            m_diagnostics.emplace_back(
                Diagnostic( Level::Error, error.what(),
                "Combine", 1, 0 ) );
        }
    }
    else {
        YAML::Node yamlNodeResult = loadYamlNode( m_generalYAMLPath );
        YAML::Node yamlNodeForAdd = loadYamlNode( yamlPathToMerge );

        for( const auto & diagnosticsIt : yamlNodeForAdd["Diagnostics"] ) {
            yamlNodeResult["Diagnostics"].push_back( diagnosticsIt );
        }

        try {
            std::ofstream mergedYamlOutputFile( m_generalYAMLPath );
            mergedYamlOutputFile << yamlNodeResult;
        }
        catch( std::exception & error ) {
            m_diagnostics.emplace_back(
                Diagnostic( Level::Error, error.what(),
                "Combine", 1, 0 ) );
        }
    }
}

YAML::Node LintCombine::LinterCombine::loadYamlNode( const std::string & pathToYaml ) {
    YAML::Node yamlNode;
    try {
        std::ifstream filePathToYaml( pathToYaml );
        if( filePathToYaml.fail() ) {
            throw std::logic_error( "YAML file path \""
                                    + pathToYaml + "\" doesn't exist" );
        }
        yamlNode = YAML::LoadFile( pathToYaml );
    }
    catch( std::exception & error ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Error, error.what(),
            "Combine", 1, 0 ) );
    }
    return yamlNode;
}
