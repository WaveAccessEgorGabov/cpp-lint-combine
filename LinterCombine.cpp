#include "LinterCombine.h"

#include <vector>
#include <memory>
#include <stdexcept>

char ** LintCombine::LinterCombine::vectorStringToCharPP( const std::vector < std::string > & stringVector ) {
    char ** str = new char * [stringVector.size()];
    for( int i = 0; i < stringVector.size(); ++i )
        str[ i ] = const_cast< char * > ( stringVector[ i ].c_str() );
    return str;
}

LintCombine::LinterCombine::LinterCombine( int argc, char ** argv, FactoryBase & factory )
        : service( factory.getService() ) {
    std::vector < std::vector < std::string > > lintersVectorString = splitCommandLineByLinters( argc, argv );
    for( const auto & it_extern : lintersVectorString ) {
        char ** lintersCharPP = vectorStringToCharPP( it_extern );
        std::shared_ptr < LinterItf > linter = factory.createLinter( it_extern.size(), lintersCharPP );
        delete[] ( lintersCharPP );
        if( linter == nullptr ) {
            throw std::logic_error( "Linter is not exists" );
        }
        linters.emplace_back( linter );
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
    for( const auto & it : linters ) {
        it->callLinter();
    }
}

int LintCombine::LinterCombine::waitLinter() {
    int lintersReturnCode = 1;
    service.getIO_Service().run();
    for( const auto & it : linters ) {
        it->waitLinter() == 0 ? ( lintersReturnCode &= ~1 ) : ( lintersReturnCode |= 2 );
    }
    return lintersReturnCode;
}

LintCombine::CallTotals LintCombine::LinterCombine::updateYaml() const {
    CallTotals callTotals;
    for( const auto & it : linters ) {
        callTotals += it->updateYaml();
    }
    return callTotals;
}

std::shared_ptr < LintCombine::LinterItf > LintCombine::LinterCombine::linterAt( int pos ) const {
    if( pos >= linters.size() )
        throw std::out_of_range( "index out of bounds" );
    return linters[ pos ];
}

int LintCombine::LinterCombine::numLinters() const noexcept {
    return linters.size();
}

