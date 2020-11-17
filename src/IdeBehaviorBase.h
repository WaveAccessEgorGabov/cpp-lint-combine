#pragma once

#include "IdeBehaviorItf.h"

namespace LintCombine {
    class IdeBehaviorBase final : public IdeBehaviorItf {
    public:
        IdeBehaviorBase( const bool mergeStdoutAndStderrVal,
                         const bool mayYamlFileContainDocLinkVal,
                         const bool linterExitCodeTolerantVal )
            : m_mergeStdoutAndStderr( mergeStdoutAndStderrVal ),
            m_mayYamlFileContainDocLink( mayYamlFileContainDocLinkVal ),
            m_linterExitCodeTolerant( linterExitCodeTolerantVal ) {}

        bool doesMergeStdoutAndStderr() const override;

        bool mayYamlFileContainDocLink() const override;

        bool isLinterExitCodeTolerant() const override;

    private:
        bool m_mergeStdoutAndStderr;
        bool m_mayYamlFileContainDocLink;
        bool m_linterExitCodeTolerant;
    };
}
