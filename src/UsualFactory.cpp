#include "ClangTidyWrapper.h"
#include "ClazyWrapper.h"
#include "UsualFactory.h"

std::shared_ptr < LintCombine::LinterItf >
LintCombine::UsualFactory::createLinter( stringVectorConstRef subLinterCommandLine ) {
    if( subLinterCommandLine[ 0 ] == "clang-tidy" ) {
        return std::make_shared < LintCombine::ClangTidyWrapper >
                ( stringVector( subLinterCommandLine.begin() + 1, subLinterCommandLine.end() ),
                  this->getServices() );
    }

    if( subLinterCommandLine[ 0 ] == "clazy" ) {
        return std::make_shared < LintCombine::ClazyWrapper >
                ( stringVector( subLinterCommandLine.begin() + 1, subLinterCommandLine.end() ),
                  this->getServices() );
    }

    return nullptr;
}
