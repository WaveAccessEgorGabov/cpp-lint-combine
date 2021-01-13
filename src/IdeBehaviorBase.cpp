#include "IdeBehaviorBase.h"

bool LintCombine::IdeBehaviorBase::doesConvertLinterOutput() const {
    return m_convertLinterOutput;
}

bool LintCombine::IdeBehaviorBase::doesConvertLinterOutputEncoding() const {
    return m_convertLinterOutputEncoding;
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

void LintCombine::IdeBehaviorBase::setExtraOptions( const bool convertLinterOutputEncodingVal ) {
    m_convertLinterOutputEncoding = convertLinterOutputEncodingVal;
}
