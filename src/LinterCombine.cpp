#include "LinterCombine.h"
#include "../data/version.rsrc"

#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>
#include <boost/filesystem.hpp>

LintCombine::LinterCombine::LinterCombine( stringVectorConstRef commandLineSTL,
                                           LintCombine::FactoryBase & factory ) : services( factory.getServices() ) {
}

LintCombine::LinterCombine::LinterCombine( int argc, char ** argv, FactoryBase & factory )
        : services( factory.getServices() ) {
    std::vector < std::vector < std::string > > subLintersCommandLineVV = splitCommandLineBySubLinters( argc, argv );
    for( const auto & subLinterIt : subLintersCommandLineVV ) {
        char ** subLintersCommandLineCharPP = vectorStringToCharPP( subLinterIt );
        std::shared_ptr < LinterItf > subLinter
                = factory.createLinter( static_cast < int > ( subLinterIt.size() ), subLintersCommandLineCharPP );
        delete[] ( subLintersCommandLineCharPP );
        if( subLinter == nullptr ) {
            throw std::logic_error( "Linter is not exists" );
        }
        m_linters.emplace_back( subLinter );
    }
}

void LintCombine::LinterCombine::callLinter() {
    // to continue work's of io_service
    services.getIO_Service().restart();
    for( const auto & subLinterIt : m_linters ) {
        subLinterIt->callLinter();
    }
}

int LintCombine::LinterCombine::waitLinter() {
    if( m_linters.empty() ) {
        return 0;
    }
    int returnCode = 1;
    services.getIO_Service().run();
    for( const auto & subLinterIt : m_linters ) {
        subLinterIt->waitLinter() == 0 ? ( returnCode &= ~1 ) : ( returnCode |= 2 );
    }
    return returnCode;
}

const std::string & LintCombine::LinterCombine::getYamlPath() {
    if( !m_mergedYamlPath.empty() ) {
        for( const auto & subLinterIt : m_linters ) {
            if( !subLinterIt->getYamlPath().empty() ) {
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

/* ToDo:
 * Print only name and version?
 * Or also program options (by using boost::program_options::options_description)?
*/
bool LintCombine::LinterCombine::printTextIfRequested() const {
    if( m_helpIsRequested ) {
        std::cout << "Product name: " << PRODUCTNAME_STR << std::endl;
        std::cout << "Product version: " << PRODUCTVERSION_STR << std::endl;
    }
    return m_helpIsRequested;
}

/* ToDo:
 * research - Does boost::program_options useable here?
*/
std::vector < std::vector < std::string > >
LintCombine::LinterCombine::splitCommandLineBySubLinters( int argc, char ** argv ) {
    std::vector < std::vector < std::string > > subLinterVec;
    std::vector < std::string > currentSubLinter;
    for( int i = 0; i < argc; ++i ) {
        std::string argvAsString( argv[ i ] );
        if( !argvAsString.find( "--sub-linter=" ) ) {
            // if "--sub-linter=" is found again, add current sub-linter with options to subLinterVec
            if( !currentSubLinter.empty() ) {
                subLinterVec.emplace_back( currentSubLinter );
                currentSubLinter.clear();
            }
            currentSubLinter.emplace_back(
                    argvAsString.substr( std::string( "--sub-linter=" ).size(), argvAsString.size() ) );
        }
            // skip options before "--sub-linter="
        else if( !currentSubLinter.empty() ) {
            currentSubLinter.emplace_back( argvAsString );
        }
        else if( !argvAsString.find( "--help" ) || !argvAsString.find( "-h" ) ) {
            m_helpIsRequested = true;
        }
        else if( !argvAsString.find( "--resultYaml=" ) ) {
            m_mergedYamlPath = argvAsString.substr( std::string( "--export-fixes=" ).size(), argvAsString.size() );
            std::string mergedYamlFileName = boost::filesystem::path( m_mergedYamlPath ).filename().string();
            if( !boost::filesystem::portable_name( mergedYamlFileName ) ) {
                std::cerr << mergedYamlFileName << " invalid!" << std::endl;
                m_mergedYamlPath = std::string();
            }
        }

        if( i == argc - 1 && !currentSubLinter.empty() ) {
            subLinterVec.emplace_back( currentSubLinter );
        }
    }
    return subLinterVec;
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

char ** LintCombine::LinterCombine::vectorStringToCharPP( const std::vector < std::string > & stringVector ) {
    char ** str = new char * [stringVector.size()];
    for( size_t i = 0; i < stringVector.size(); ++i )
        str[ i ] = const_cast< char * > ( stringVector[ i ].c_str() );
    return str;
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
