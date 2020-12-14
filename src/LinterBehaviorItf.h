#pragma once

#include <boost/process.hpp>

#include <string>

namespace LintCombine {
    class LinterBehaviorItf {
    public:
        virtual ~LinterBehaviorItf() = default;

        virtual std::streamsize convertLinterOutput( std::string & linterOutputPart ) = 0;

        virtual boost::process::child
        createProcess( const bool doesMergeStdoutAndStderr, const std::string & name,
                       const std::string & yamlPath, const std::string & options,
                       boost::process::async_pipe & stdoutPipe,
                       boost::process::async_pipe & stderrPipe ) const = 0;
    };
}
