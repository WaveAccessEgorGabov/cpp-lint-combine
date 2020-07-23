#include "ClangTidyWrapper.h"
#include "ClazyWrapper.h"
#include "UsualLinterFactory.h"

std::shared_ptr < LintCombine::LinterItf >
LintCombine::UsualLinterFactory::createLinter( const stringVector & subLinterCommandLine ) {
    if( *subLinterCommandLine.begin() == "clang-tidy" ) {
        return std::make_shared < ClangTidyWrapper >
            ( stringVector( subLinterCommandLine.begin() + 1, subLinterCommandLine.end() ),
              this->getServices() );
    }

    if( *subLinterCommandLine.begin() == "clazy" ) {
        return std::make_shared < ClazyWrapper >
            ( stringVector( subLinterCommandLine.begin() + 1, subLinterCommandLine.end() ),
              this->getServices() );
    }
    // TODO: mayBe add diagnostic?
    return nullptr;
}
