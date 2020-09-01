#include "ClangTidyWrapper.h"
#include "ClazyWrapper.h"
#include "UsualLinterFactory.h"
#include "LinterCombine.h"

std::shared_ptr< LintCombine::LinterItf >
LintCombine::UsualLinterFactory::createLinter( const stringVector & cmdLine ) {
    if( !cmdLine.empty() && *cmdLine.begin() == "clang-tidy" ) {
        return std::make_shared< ClangTidyWrapper >
            ( stringVector( cmdLine.begin() + 1, cmdLine.end() ),
              this->getServices() );
    }

    if( !cmdLine.empty() && *cmdLine.begin() == "clazy" ) {
        return std::make_shared< ClazyWrapper >
            ( stringVector( cmdLine.begin() + 1, cmdLine.end() ),
              this->getServices() );
    }

    if( !cmdLine.empty() && *cmdLine.begin() == "LinterCombine" ) {
        return std::make_shared< LinterCombine >
            ( stringVector( cmdLine.begin() + 1, cmdLine.end() ) );
    }

    return nullptr;
}
