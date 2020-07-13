#include "PrepareCmdLineVerbatim.h"

#include <boost/filesystem/path.hpp>

// return "stringVector()" on error
LintCombine::stringVector LintCombine::PrepareCmdLineVerbatim::transform( stringVector commandLine ) {
    return stringVector();
}

std::vector<LintCombine::Diagnostic> LintCombine::PrepareCmdLineVerbatim::diagnostics() {
    return std::vector< Diagnostic >();
}

// TODO: parse sub-linter, result-yaml
bool LintCombine::PrepareCmdLineVerbatim::parseSourceCmdLine() {
    return false;
}

// TODO: check that sub-linter exists, check that result-yaml exists or set own path
bool LintCombine::PrepareCmdLineVerbatim::validateParsedData() {
    return false;
}

// If path to result-yaml exists, check for correctness
bool LintCombine::PrepareCmdLineVerbatim::checkYamlPathForCorrectness(){
    return false;
}




