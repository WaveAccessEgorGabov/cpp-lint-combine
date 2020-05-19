#include "LinterCombine.h"
#include "version.rsrc"

#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>

char ** LintCombine::LinterCombine::vectorStringToCharPP( const std::vector < std::string > & stringVector ) {
    char ** str = new char * [stringVector.size()];
    for( size_t i = 0; i < stringVector.size(); ++i )
        str[ i ] = const_cast< char * > ( stringVector[ i ].c_str() );
    return str;
}

bool LintCombine::LinterCombine::printTextIfRequested() const {
    if( m_helpIsRequested ) {
        std::cout << "-----------------------------------------------------------------------------------" << std::endl;
        std::cout << "Product name: " << PRODUCTNAME_STR << std::endl;
        std::cout << "Product version: " << PRODUCTVERSION_STR << std::endl;
        std::cout << "-----------------------------------------------------------------------------------" << std::endl;
    }
    return m_helpIsRequested;
}

LintCombine::LinterCombine::LinterCombine( int argc, char ** argv, FactoryBase & factory )
        : service( factory.getService() ) {
    std::vector < std::vector < std::string > > lintersVectorString = splitCommandLineByLinters( argc, argv );
    for( const auto & it_extern : lintersVectorString ) {
        char ** lintersCharPP = vectorStringToCharPP( it_extern );
        std::shared_ptr < LinterItf > linter
    		= factory.createLinter( static_cast < int > ( it_extern.size() ), lintersCharPP );
        delete[] ( lintersCharPP );
        if( linter == nullptr ) {
            throw std::logic_error( "Linter is not exists" );
        }
        m_linters.emplace_back( linter );
    }
}

std::vector < std::vector < std::string > >
LintCombine::LinterCombine::splitCommandLineByLinters( int argc, char ** argv ) {
    std::vector < std::vector < std::string > > linterDataVec;
    std::vector < std::string > linterData;
    for( int i = 0; i < argc; ++i ) {
        std::string argvAsString( argv[ i ] );
        if( !argvAsString.find( "--sub-linter=" ) ) {
            // if "--sub-linter=" is found again, add current sub-linter with options to linterDataVec
            if( !linterData.empty() ) {
                linterDataVec.emplace_back( linterData );
                linterData.clear();
            }
            // + size() for skipping "--sub-linter=" and add only sub-linter name
            linterData.emplace_back( argvAsString.c_str() + std::string( "--sub-linter=" ).size() );
        }
            // skip options before "--sub-linter="
        else if( !linterData.empty() ) {
            linterData.emplace_back( argvAsString );
        } else if( !argvAsString.find( "--help" ) || !argvAsString.find( "-h" ) ) {
            m_helpIsRequested = true;
        }

        if( i == argc - 1 && !linterData.empty() ) {
            linterDataVec.emplace_back( linterData );
        }
    }
    return linterDataVec;
}

void LintCombine::LinterCombine::callLinter() {
    // to continue work of io_service
    service.getIO_Service().restart();
    for( const auto & it : m_linters ) {
        it->callLinter();
    }
}

int LintCombine::LinterCombine::waitLinter() {
    if ( !m_linters.size() ) {
        return 0;
    }
	int lintersReturnCode = 1;
    service.getIO_Service().run();
    for( const auto & it : m_linters ) {
        it->waitLinter() == 0 ? ( lintersReturnCode &= ~1 ) : ( lintersReturnCode |= 2 );
    }
    return lintersReturnCode;
}

LintCombine::CallTotals LintCombine::LinterCombine::updateYaml() const {
    CallTotals callTotals;
    for( const auto & it : m_linters ) {
        callTotals += it->updateYaml();
    }
    return callTotals;
}

std::shared_ptr < LintCombine::LinterItf > LintCombine::LinterCombine::linterAt( const int pos ) const {
    if ( pos >= static_cast <int> ( m_linters.size() ) )
        throw std::out_of_range( "index out of bounds" );
    return m_linters[ pos ];
}

size_t LintCombine::LinterCombine::numLinters() const noexcept {
    return m_linters.size();
}

