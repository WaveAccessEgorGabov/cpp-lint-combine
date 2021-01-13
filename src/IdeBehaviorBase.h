#pragma once

#include "IdeBehaviorItf.h"

namespace LintCombine {
    class IdeBehaviorBase final : public IdeBehaviorItf {
    public:
        IdeBehaviorBase( const bool convertLinterOutputVal,
                         const bool mergeStdoutAndStderrVal,
                         const bool mayYamlFileContainDocLinkVal,
                         const bool linterExitCodeTolerantVal )
            : m_convertLinterOutput( convertLinterOutputVal ),
              m_convertLinterOutputEncoding( false ),
              m_mergeStdoutAndStderr( mergeStdoutAndStderrVal ),
              m_mayYamlFileContainDocLink( mayYamlFileContainDocLinkVal ),
              m_linterExitCodeTolerant( linterExitCodeTolerantVal ) {}

        bool doesConvertLinterOutput() const override;

        bool doesConvertLinterOutputEncoding() const override;

        bool doesMergeStdoutAndStderr() const override;

        bool mayYamlFileContainDocLink() const override;

        bool isLinterExitCodeTolerant() const override;

        void setExtraOptions( const bool convertLinterOutputEncodingVal ) override;

    private:
        bool m_convertLinterOutput;
        bool m_convertLinterOutputEncoding;
        bool m_mergeStdoutAndStderr;
        bool m_mayYamlFileContainDocLink;
        bool m_linterExitCodeTolerant;
    };
}
