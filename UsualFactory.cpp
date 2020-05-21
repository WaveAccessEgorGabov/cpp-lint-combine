#include "ClangTidyWrapper.h"
#include "ClazyWrapper.h"
#include "UsualFactory.h"

std::shared_ptr < LintCombine::LinterItf >
LintCombine::UsualFactory::createLinter( int argc, char ** argv ) {
    if( !strcmp( argv[ 0 ], "clang-tidy" ) ) {
        return std::make_shared < LintCombine::ClangTidyWrapper >( argc, argv, getServices() );
    }

    if( !strcmp( argv[ 0 ], "clazy-standalone" ) ) {
        return std::make_shared < LintCombine::ClazyWrapper >( argc, argv, getServices() );
    }

    return nullptr;
}
