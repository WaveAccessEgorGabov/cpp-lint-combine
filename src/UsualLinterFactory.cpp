#include "ClangTidyWrapper.h"
#include "ClazyWrapper.h"
#include "UsualLinterFactory.h"
#include "LinterCombine.h"

std::unique_ptr< LintCombine::LinterItf >
LintCombine::UsualLinterFactory::createLinter( const stringVector & cmdLine ) {
    if( !cmdLine.empty() && *cmdLine.begin() == "clang-tidy" ) {
        return std::make_unique< ClangTidyWrapper >
            ( stringVector( cmdLine.begin() + 1, cmdLine.end() ),
            this->getServices() );
    }

    if( !cmdLine.empty() && *cmdLine.begin() == "clazy" ) {
        return std::make_unique< ClazyWrapper >
            ( stringVector( cmdLine.begin() + 1, cmdLine.end() ),
            this->getServices() );
    }

    if( !cmdLine.empty() && *cmdLine.begin() == "LinterCombine" ) {
        return std::make_unique< LinterCombine >
            ( stringVector( cmdLine.begin() + 1, cmdLine.end() ) );
    }

    return nullptr;
}
