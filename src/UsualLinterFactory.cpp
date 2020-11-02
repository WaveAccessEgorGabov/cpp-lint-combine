#include "ClangTidyBehavior.h"
#include "ClangTidyWrapper.h"
#include "ClazyBehavior.h"
#include "ClazyWrapper.h"
#include "UsualLinterFactory.h"
#include "LinterCombine.h"

std::unique_ptr< LintCombine::LinterItf >
LintCombine::UsualLinterFactory::createLinter( const StringVector & cmdLine ) {
    if( !cmdLine.empty() && *cmdLine.begin() == "clang-tidy" ) {
        return std::make_unique< ClangTidyWrapper >(
            StringVector( cmdLine.begin() + 1, cmdLine.end() ), this->getServices(),
                          std::make_unique< ClangTidyBehavior >() );
    }

    if( !cmdLine.empty() && *cmdLine.begin() == "clazy" ) {
        return std::make_unique< ClazyWrapper >(
            StringVector( cmdLine.begin() + 1, cmdLine.end() ), this->getServices(),
                          std::make_unique< ClazyBehavior >() );
    }

    if( !cmdLine.empty() && *cmdLine.begin() == "LinterCombine" ) {
        return std::make_unique< LinterCombine >(
            StringVector( cmdLine.begin() + 1, cmdLine.end() ) );
    }

    return nullptr;
}
