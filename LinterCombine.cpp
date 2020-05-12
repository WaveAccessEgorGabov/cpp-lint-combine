#include "LinterCombine.h"

#include <vector>
#include <memory>
#include <stdexcept>

static char ** vectorStringToCharPP( const std::vector < std::string > & stringVector ) {
    char ** str = ( char ** ) malloc( stringVector.size() * sizeof( char * ) );
    for( int i = 0; i < stringVector.size(); ++i )
        str[ i ] = const_cast<char *> (stringVector[ i ].c_str());
    return str;
}

LintCombine::LinterCombine::LinterCombine( int argc, char ** argv, FactoryBase & factory )
        : service( factory.getService() ) {
    std::vector < std::vector < std::string > > lintersVectorString = splitCommandLineByLinters( argc, argv );
    for( const auto & it_extern : lintersVectorString ) {
        char ** lintersCharPP = vectorStringToCharPP( it_extern );
        std::shared_ptr < LinterItf > linter = factory.createLinter( it_extern.size(), lintersCharPP );
        free( lintersCharPP );
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
            if( !linterData.empty() ) {
                linterDataVec.emplace_back( linterData );
                linterData.clear();
            }
            if( i == argc - 1 ) {
                linterData.emplace_back( argvAsString.c_str() + std::string( "--sub-linter=" ).size() );
                linterDataVec.emplace_back( linterData );
            }
            linterData.emplace_back( argvAsString.c_str() + std::string( "--sub-linter=" ).size() );
        } else if( !linterData.empty() ) {
            linterData.emplace_back( argvAsString );
            if( i == argc - 1 ) {
                linterDataVec.emplace_back( linterData );
            }
        }
    }
    return linterDataVec;
}

void LintCombine::LinterCombine::callLinter() {
    service.getIO_Service().restart();
    for( auto it : linters ) {
        it->callLinter();
    }
}

int LintCombine::LinterCombine::waitLinter() {
    int lintersReturnCode = 1;
    service.getIO_Service().run();
    for( auto it : linters ) {
        it->waitLinter() == 0 ? lintersReturnCode &= ~1 : lintersReturnCode |= 2;
    }
    return lintersReturnCode;
}

LintCombine::CallTotals LintCombine::LinterCombine::updateYaml() {
    CallTotals callTotals;
    for(auto it: linters) {
        callTotals += it->updateYaml();
    }
    return callTotals;
}

std::shared_ptr < LintCombine::LinterItf > LintCombine::LinterCombine::linterAt( int pos ) {
    return linters[ pos ];
}

int LintCombine::LinterCombine::numLinters() {
    return linters.size();
}

