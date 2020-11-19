#pragma once

namespace LintCombine {
    struct IdeBehaviorItf {
        virtual bool doesMergeStdoutAndStderr() const = 0;
        virtual bool doesConvertLinterOutput() const = 0;
        virtual bool mayYamlFileContainDocLink() const = 0;
        virtual bool isLinterExitCodeTolerant() const = 0;
        virtual ~IdeBehaviorItf() = default;
    };
}