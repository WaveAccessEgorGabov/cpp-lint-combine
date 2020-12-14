#include "ClazyBehavior.h"

#include <regex>

std::streamsize LintCombine::ClazyBehavior::convertLinterOutput( std::string & linterOutputPart ) {
    static const std::string path = "([\\\\/][^\"*?\\\\/<>:|]*?)";
    static const std::string rowColumn = "\\((\\d+),(\\d+)\\):";
    static const std::string levelAndMessage = "( (?:warning|error): .*? \\[)";
    static const std::string hyphens = "(-)*";
    static const std::string name = "(.*?\\])";
    static const std::regex s_conform(
        path + rowColumn + levelAndMessage + hyphens + name, std::regex::optimize
    );
    std::string convertedOutput;
    std::smatch match;
    while( std::regex_search( linterOutputPart, match, s_conform ) ) {
        if( match[match.size() - 1].matched ) {
            convertedOutput +=
                match.prefix().str() +
                std::regex_replace( match[0].str(), s_conform, "$1:$2:$3:$4$6" );
            linterOutputPart = match.suffix();
        }
    }
    static const std::regex s_conformPath( path, std::regex::optimize );
    if( std::regex_search( linterOutputPart, match, s_conformPath ) ) {
        convertedOutput += match.prefix().str();
        linterOutputPart = convertedOutput + match[0].str() + match.suffix().str();
        return convertedOutput.size();
    }
    linterOutputPart = convertedOutput + linterOutputPart;
    return linterOutputPart.size();
}

boost::process::child LintCombine::ClazyBehavior::createProcess(
        const bool doesMergeStdoutAndStderr,
        const std::string & name, const std::string & yamlPath, const std::string & options,
        boost::process::async_pipe & stdoutPipe,
        boost::process::async_pipe & stderrPipe ) const {
    std::string runCommand;
    if( !yamlPath.empty() )
        runCommand = name + " --export-fixes=" + yamlPath + " " + options;
    else
        runCommand = name + " " + options;
    if( doesMergeStdoutAndStderr ) {
        return boost::process::child( runCommand,
                                      ( boost::process::std_out & boost::process::std_err ) > stdoutPipe );
    }
    return boost::process::child( runCommand,
                                  boost::process::std_out > stdoutPipe,
                                  boost::process::std_err > stderrPipe );
}
