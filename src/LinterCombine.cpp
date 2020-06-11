#include "LinterCombine.h"
#include "../data/version.rsrc"

#include <iostream>
#include <boost/filesystem.hpp>

LintCombine::LinterCombine::LinterCombine( const stringVector & commandLine, LintCombine::FactoryBase & factory )
        : m_services( factory.getServices() ) {
    std::vector < stringVector > subLintersCommandLine = splitCommandLineBySubLinters( commandLine );
    if( m_helpIsRequested ) {
        return;
    }
    for( const auto & it : subLintersCommandLine ) {
        std::shared_ptr < LinterItf > subLinter = factory.createLinter( it );
        if( subLinter == nullptr ) {
            std::cerr << "Linter is not exists!" << std::endl;
            throw std::logic_error( "Linter is not exists!" );
        }
        m_linters.emplace_back( subLinter );
    }
    if( m_linters.empty() ) {
        std::cerr << "Warning not one linter was set!" << std::endl;
    }
    checkYamlPathForCorrectness();
}

void LintCombine::LinterCombine::callLinter() {
    // to continue work's of io_service
    m_services.getIO_Service().restart();
    for( const auto & subLinterIt : m_linters ) {
        subLinterIt->callLinter();
    }
}

int LintCombine::LinterCombine::waitLinter() {
    if( m_linters.empty() ) { return 0; }
    uint returnCode = 1;
    m_services.getIO_Service().run();
    for( const auto & subLinterIt : m_linters ) {
        subLinterIt->waitLinter() == 0 ? ( returnCode &= ~(uint)1 ) : ( returnCode |= (uint)2 );
    }
    return returnCode;
}

const std::string & LintCombine::LinterCombine::getYamlPath() {
    if( !m_mergedYamlPath.empty() ) {
        for( const auto & subLinterIt : m_linters ) {
            if( !subLinterIt->getYamlPath().empty() && boost::filesystem::exists( subLinterIt->getYamlPath() ) ) {
                try {
                    mergeYaml( subLinterIt->getYamlPath() );
                }
                catch( std::exception & ex ) {
                    std::cerr << "Exception while merge yaml. What(): " << ex.what() << std::endl;
                }
            }
        }
    }
    if( boost::filesystem::exists( m_mergedYamlPath ) )
        return m_mergedYamlPath;
    m_mergedYamlPath.clear();
    return m_mergedYamlPath;
}

LintCombine::CallTotals LintCombine::LinterCombine::updateYaml() const {
    CallTotals callTotals;
    for( const auto & subLinterIt : m_linters ) {
        callTotals += subLinterIt->updateYaml();
    }
    return callTotals;
}

std::shared_ptr < LintCombine::LinterItf > LintCombine::LinterCombine::linterAt( const int pos ) const {
    if( pos >= static_cast < int > ( m_linters.size() ) )
        throw std::out_of_range( "index out of bounds" );
    return m_linters[ pos ];
}

size_t LintCombine::LinterCombine::numLinters() const noexcept {
    return m_linters.size();
}

bool LintCombine::LinterCombine::printTextIfRequested() const {
    if( m_helpIsRequested ) {
        std::cout << "Product name: " << PRODUCTNAME_STR << std::endl;
        std::cout << "Product version: " << PRODUCTVERSION_STR << std::endl;
        std::cout << "Program options: " << std::endl;
        std::cout << m_genericOptDesc << std::endl;
    }
    return m_helpIsRequested;
}

std::vector < LintCombine::stringVector >
LintCombine::LinterCombine::splitCommandLineBySubLinters( const stringVector & commandLine ) {
    stringVector lintersName;
    m_genericOptDesc.add_options()
            ( "help", "print this message" )
            ( "result-yaml", boost::program_options::value < std::string >( & m_mergedYamlPath ),
              "path to yaml with diagnostics from all linters" )
            ( "sub-linter",
              boost::program_options::value < std::vector < std::string > >( & lintersName ),
              "linter to use" );

    boost::program_options::variables_map vm;
    boost::program_options::store(
            boost::program_options::command_line_parser( commandLine ).
                    options( m_genericOptDesc ).allow_unregistered().run(), vm );
    boost::program_options::notify( vm );

    if( vm.count( "help" ) ) {
        m_helpIsRequested = true;
    }

    stringVector currentSubLinter;
    std::vector < stringVector > subLintersVec;
    for( size_t i = 0, linterNum = 0; i < commandLine.size(); ++i ) {
        if( linterNum != lintersName.size() && commandLine[ i ] == "--sub-linter=" + lintersName[ linterNum ] ) {
            if( linterNum != 0 ) {
                subLintersVec.emplace_back( currentSubLinter );
                currentSubLinter.clear();
            }
            currentSubLinter.emplace_back( lintersName[ linterNum ] );
            if( i == commandLine.size() - 1 ) {
                subLintersVec.emplace_back( currentSubLinter );
            }
            ++linterNum;
        }
        else if( !currentSubLinter.empty() ) {
            currentSubLinter.emplace_back( commandLine[ i ] );
            if( i == commandLine.size() - 1 ) {
                subLintersVec.emplace_back( currentSubLinter );
            }
        }
    }
    return subLintersVec;
}

void LintCombine::LinterCombine::checkYamlPathForCorrectness() {
    std::string yamlFileName = boost::filesystem::path( m_mergedYamlPath ).filename().string();
    if( !boost::filesystem::portable_name( yamlFileName ) ) {
        std::cerr << "\"" << yamlFileName << "\" is incorrect file name! (target yaml-path incorrect)" << std::endl;
        m_mergedYamlPath = std::string();
    }
}

void LintCombine::LinterCombine::mergeYaml( const std::string & yamlPathToMerge ) const {
    if( !boost::filesystem::exists( m_mergedYamlPath ) ) {
        try {
            boost::filesystem::copy( yamlPathToMerge, m_mergedYamlPath );
        }
        catch( const boost::filesystem::filesystem_error & ex ) {
            std::cerr << "boost::filesystem::filesystem_error exception while merging. What(): "
                      << ex.what() << std::endl;
        }
        catch( std::exception & ex ) {
            std::cerr << "Exception while merging. What(): " << ex.what() << std::endl;
        }
    }
    else {
        YAML::Node yamlNodeResult = loadYamlNode( m_mergedYamlPath );
        YAML::Node yamlNodeForAdd = loadYamlNode( yamlPathToMerge );

        for( const auto & diagnosticsIt : yamlNodeForAdd[ "Diagnostics" ] ) {
            yamlNodeResult[ "Diagnostics" ].push_back( diagnosticsIt );
        }

        try {
            std::ofstream mergedYamlOutputFile( m_mergedYamlPath );
            mergedYamlOutputFile << yamlNodeResult;
        }
        catch( const std::ios_base::failure & ex ) {
            std::cerr << "std::ios_base::failure exception while write result yaml. What(): " << ex.what() << std::endl;
        }
        catch( std::exception & ex ) {
            std::cerr << "Exception while write result yaml. What(): " << ex.what() << std::endl;
        }
    }
}

YAML::Node LintCombine::LinterCombine::loadYamlNode( const std::string & pathToYaml ) {
    YAML::Node yamlNode;
    try {
        yamlNode = YAML::LoadFile( pathToYaml );
    }
    catch( const YAML::BadFile & ex ) {
        std::cerr << "YAML::BadFile exception while load mergedYamlPath. What(): " << ex.what() << std::endl;
    }
    catch( std::exception & ex ) {
        std::cerr << "Exception while load mergedYamlPath. What(): " << ex.what() << std::endl;
    }
    return yamlNode;
}
