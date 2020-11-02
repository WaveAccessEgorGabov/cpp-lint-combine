#include "ClangTidyBehavior.h"

std::string LintCombine::ClangTidyBehavior::convertLinterOutput( std::string && linterOutputPart,
                                                                 const ReadLinterOutputFrom ) {
    return linterOutputPart;
}
