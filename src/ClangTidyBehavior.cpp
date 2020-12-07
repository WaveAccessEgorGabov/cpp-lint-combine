#include "ClangTidyBehavior.h"

#ifdef WIN32
#include "LintCombineUtils.h"
#endif

std::streamsize LintCombine::ClangTidyBehavior::convertLinterOutput( std::string & ) {
    return -1;
}

boost::process::child LintCombine::ClangTidyBehavior::createProcess(
        const bool doesMergeStdoutAndStderr,
        const std::string & name, const std::string & yamlPath, const std::string & options,
        boost::process::async_pipe & stdoutPipe,
        boost::process::async_pipe & stderrPipe ) const {
    std::string runCommand;
    if( !yamlPath.empty() )
        runCommand = name + " --export-fixes=" + yamlPath + " " + options;
    else
        runCommand = name + " " + options;
    auto runCommandInRequiredEncoding =
    #ifdef WIN32
        Utf8ToUtf16( runCommand );
    #else
        runCommand;
    #endif
    if( doesMergeStdoutAndStderr ) {
        return boost::process::child( runCommandInRequiredEncoding,
                                      ( boost::process::std_out & boost::process::std_err ) > stdoutPipe );
    }
    return boost::process::child( runCommandInRequiredEncoding,
                                  boost::process::std_out > stdoutPipe,
                                  boost::process::std_err > stderrPipe );
}
