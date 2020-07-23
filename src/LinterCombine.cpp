#include "LinterCombine.h"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

// TODO: parse sub-linters and result-yaml separately

std::vector<LintCombine::Diagnostic> LintCombine::LinterCombine::diagnostics() {
    std::vector< Diagnostic > allDiagnostics;
    for( const auto & subLinterIt : m_linters ) {
        const auto & diags = subLinterIt->diagnostics();
        allDiagnostics.insert( allDiagnostics.end(),
                               diags.begin(), diags.end() );
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
        isErrorOccur = true;
        return;
    }

    std::vector < stringVector > subLintersCommandLine =
        splitCommandLineBySubLinters( cmdLine );
    if( isErrorOccur ) {
        return;
    }

    if( subLintersCommandLine.empty() ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Error,
            "No one linter was parsed",
            "Combine", 1, 0 ) );
        isErrorOccur = true;
        return;
    }

    for( const auto & it : subLintersCommandLine ) {
        const auto subLinter = factory.createLinter( it );
        if( subLinter == nullptr ) {
            m_diagnostics.emplace_back(
                Diagnostic( Level::Error,
                "Unknown linter name: \"" + *it.begin() + "\"",
                "Combine", 1, 0 ) );
            isErrorOccur = true;
        }
        else {
            m_linters.emplace_back( subLinter );
        }
    }

    if( isErrorOccur ) {
        m_linters.clear();
        return;
    }

    validateGeneralYamlPath( cmdLine );
}

void LintCombine::LinterCombine::callLinter() {
    // to continue work's of io_service
    m_services.getIO_Service().restart();
    for( const auto & subLinterIt : m_linters ) {
        subLinterIt->callLinter();
    }
}

int LintCombine::LinterCombine::waitLinter() {
    if( m_linters.empty() ) {
        return 0;
    }
    int returnCode = 1;
    m_services.getIO_Service().run();
    for( const auto & subLinterIt : m_linters ) {
        subLinterIt->waitLinter() == 0 ? ( returnCode &= ~1 ) :
            ( returnCode |= 2 );
    }
    if( returnCode == 2 ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Warning,
            "Some linters are failed while running",
            "Combine", 1, 0 ) );
    }
    if( returnCode == 3 ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Error,
            "All linters are failed while running",
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
            + " yaml-files was failed", "Combine", 1, 0 ) );
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
        isErrorOccur = true;
        return std::vector < stringVector >();
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

// TODO: May be move to LinterUtils
void LintCombine::LinterCombine::validateGeneralYamlPath( const stringVector & cmdLine ) {
    const std::string pathToGeneralYamlOnError =
        CURRENT_BINARY_DIR "LintersDiagnostics.yaml";
    boost::program_options::options_description genYamlOptDesc;
    genYamlOptDesc.add_options()
        ( "result-yaml",
          boost::program_options::value < std::string >( &m_pathToGeneralYaml )
          ->default_value( pathToGeneralYamlOnError ) );
    boost::program_options::variables_map vm;
    try {
        store( boost::program_options::command_line_parser( cmdLine ).
                options( genYamlOptDesc ).allow_unregistered().run(), vm );
        notify( vm );
    }
    catch( const std::exception & error ) {
        m_diagnostics.emplace_back( Diagnostic( Level::Warning, error.what(),
                                    "Combine", 1, 0 ) );
        m_diagnostics.emplace_back( Diagnostic(
            Level::Info,
            "path to result-yaml changed to " + pathToGeneralYamlOnError,
            "Combine", 1, 0 ) );
        m_pathToGeneralYaml = pathToGeneralYamlOnError;
        return;
    }

    const auto yamlFilename =
        boost::filesystem::path( m_pathToGeneralYaml ).filename().string();
    if( !boost::filesystem::portable_name( yamlFilename ) ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Warning,
            "Incorrect general yaml filename: \"" + yamlFilename +
            "\"", "Combine", 1, 0 ) );
        m_diagnostics.emplace_back( Diagnostic(
            Level::Info,
            "path to result-yaml changed to " + pathToGeneralYamlOnError,
            "Combine", 1, 0 ) );
        m_pathToGeneralYaml = pathToGeneralYamlOnError;
    }
}

const std::string & LintCombine::LinterCombine::getYamlPath() {
    if( !m_pathToGeneralYaml.empty() ) {
        for( const auto & subLinterIt : m_linters ) {
            if( subLinterIt->getYamlPath().empty() ) {
                m_diagnostics.emplace_back(
                    Diagnostic( Level::Warning,
                    "linter's yaml path value is empty",
                    "Combine", 1, 0 ) );
                continue;
            }
            if( !boost::filesystem::exists( subLinterIt->getYamlPath() ) ) {
                m_diagnostics.emplace_back(
                Diagnostic( Level::Warning,
                    "linter's yaml path \"" + subLinterIt->getYamlPath()
                    + "\" doesn't exist",
                    "Combine", 1, 0 ) );
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
            "path to general yaml is empty",
            "Combine", 1, 0 ) );
    }
    if( boost::filesystem::exists( m_pathToGeneralYaml ) ) {
        return m_pathToGeneralYaml;
    }
    m_diagnostics.emplace_back(
    Diagnostic( Level::Error,
        "General yaml isn't created",
        "Combine", 1, 0 ) );
    m_pathToGeneralYaml.clear();
    return m_pathToGeneralYaml;
}

void LintCombine::LinterCombine::mergeYaml( const std::string & yamlPathToMerge ) {
    if( !boost::filesystem::exists( m_pathToGeneralYaml ) ) {
        try {
            boost::filesystem::copy( yamlPathToMerge, m_pathToGeneralYaml );
        }
        catch( std::exception & error ) {
            m_diagnostics.emplace_back(
                Diagnostic( Level::Error, error.what(),
                "Combine", 1, 0 ) );
        }
    }
    else {
        YAML::Node yamlNodeResult = loadYamlNode( m_pathToGeneralYaml );
        YAML::Node yamlNodeForAdd = loadYamlNode( yamlPathToMerge );

        for( const auto & diagnosticsIt : yamlNodeForAdd["Diagnostics"] ) {
            yamlNodeResult["Diagnostics"].push_back( diagnosticsIt );
        }

        try {
            std::ofstream mergedYamlOutputFile( m_pathToGeneralYaml );
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
        yamlNode = YAML::LoadFile( pathToYaml );
    }
    catch( std::exception & error ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Error, error.what(),
            "Combine", 1, 0 ) );
    }
    return yamlNode;
}
