#include "IdeBehaviorBase.h"

bool LintCombine::IdeBehaviorBase::doesConvertLinterOutput() const {
    return m_convertLinterOutput;
}

bool LintCombine::IdeBehaviorBase::doesMergeStdoutAndStderr() const {
    return m_mergeStdoutAndStderr;
}

bool LintCombine::IdeBehaviorBase::mayYamlFileContainDocLink() const {
    return m_mayYamlFileContainDocLink;
}

bool LintCombine::IdeBehaviorBase::isLinterExitCodeTolerant() const {
    return m_linterExitCodeTolerant;
}