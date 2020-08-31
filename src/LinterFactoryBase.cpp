#include "LinterFactoryBase.h"
#include "LinterCombine.h"

std::shared_ptr< LintCombine::LinterItf>
LintCombine::LinterFactoryBase::createLinter( const stringVector & cmdLine ) {
    for( const auto & it : cmdLine ) {
        if( it.find( "--sub-linter" ) != std::string::npos ) {
            return std::make_unique< LinterCombine >( cmdLine );
        }
    }
    throw Exception( Diagnostic( Level::Error,
                     "No linters specified. Supported linters are: clang-tidy, clazy.",
                     "LinterFactoryBase", 1, 0 ) );
}
