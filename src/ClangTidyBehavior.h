#pragma once

#include "LinterBehaviorItf.h"

namespace LintCombine {
    class ClangTidyBehavior final : public LinterBehaviorItf {
    public:
        std::streamsize convertLinterOutput( std::string & linterOutputPart ) override;

        boost::process::child
        createProcess( const bool doesMergeStdoutAndStderr, const std::string & name,
                       const std::string & yamlPath, const std::string & options,
                       boost::process::async_pipe & stdoutPipe,
                       boost::process::async_pipe & stderrPipe ) const override;
    };
}
