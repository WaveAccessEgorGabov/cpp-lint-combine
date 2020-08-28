#include "ClangTidyWrapper.h"
#include "ClazyWrapper.h"
#include "UsualLinterFactory.h"

std::shared_ptr < LintCombine::LinterItf >
LintCombine::UsualLinterFactory::createLinter( const stringVector & subLinterCmdLine ) {
    if( *subLinterCmdLine.begin() == "clang-tidy" ) {
        return std::make_shared < ClangTidyWrapper >
            ( stringVector( subLinterCmdLine.begin() + 1, subLinterCmdLine.end() ),
              this->getServices() );
    }

    if( *subLinterCmdLine.begin() == "clazy" ) {
        return std::make_shared < ClazyWrapper >
            ( stringVector( subLinterCmdLine.begin() + 1, subLinterCmdLine.end() ),
              this->getServices() );
    }
    return nullptr;
}
