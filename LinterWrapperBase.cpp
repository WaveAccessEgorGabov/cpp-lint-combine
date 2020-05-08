#include "LinterWrapperBase.h"

void LintCombine::LinterWrapperBase::callLinter() const {}

int LintCombine::LinterWrapperBase::waitLinter() const {
    return 0;
}

LintCombine::CallTotals LintCombine::LinterWrapperBase::updatedYaml() const {
    return CallTotals();
}

const std::string & LintCombine::LinterWrapperBase::getName() const {
    return name;
}

const std::string & LintCombine::LinterWrapperBase::getOptions() const {
    return options;
}

const std::string & LintCombine::LinterWrapperBase::getYamlPath() const {
    return yamlPath;
}
