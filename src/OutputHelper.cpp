#include "OutputHelper.h"
#include PATH_TO_VERSION_RESOURCE

#include <boost/algorithm/string/join.hpp>
#include <boost/program_options.hpp>

#include <iostream>

void LintCombine::OutputHelper::printHelp() {
    boost::program_options::options_description optDesc( /*line_length=*/100 );
    optDesc.add_options()
        ( "help",             "Print this message." )
        ( "ide-profile",      "Choose IDE: ReSharper C++, CLion. By default options will pass verbatim." )
        ( "result-yaml",      "Path to YAML with diagnostics from all linters." )
        ( "sub-linter",       "Linter to use (may be specified several times). "
                              "Supported linters are: clang-tidy, clazy." )
        ( "clazy-checks",     "Comma-separated list of clazy checks. Default is level1." )
        ( "clang-extra-args", "Additional argument to append to the compiler command line." );
    printProductInfo();
    std::cout << "Usage:" << std::endl << optDesc;
}

void LintCombine::OutputHelper::printHelpOption() {
    boost::program_options::options_description optDesc;
    optDesc.add_options() (
        "help", "Display all available options." );
    std::cout << optDesc;
}

void LintCombine::OutputHelper::printProductInfo() {
    std::cout << PRODUCTNAME_STR " " PRODUCTVERSION_STR << std::endl << std::endl;
}

void LintCombine::OutputHelper::setCmdLine( const StringVector & cmdLineVal ){
    m_cmdLine = cmdLineVal;
}

void LintCombine::OutputHelper::printDiagnostics( const std::vector< Diagnostic > & diagnostics ) const {
    for( const auto & diagnostic : prepareOutput( diagnostics ) ) {
        std::cerr << diagnostic << std::endl;
    }
}

LintCombine::StringVector
LintCombine::OutputHelper::prepareOutput( const std::vector< Diagnostic > & diagnostics ) const {
    auto diagnosticsCopy = diagnostics;
    std::sort( std::begin( diagnosticsCopy ), std::end( diagnosticsCopy ) );
    const auto sourceCmdLine = boost::algorithm::join( m_cmdLine, " " );
    StringVector preparedOutput;
    auto errorOccurred = false;
    for( const auto & diagnostic : diagnosticsCopy ) {
        std::string levelAsString;
        switch( diagnostic.level ) {
            case Level::Trace:   levelAsString = "Trace";   break;
            case Level::Debug:   levelAsString = "Debug";   break;
            case Level::Info:    levelAsString = "Info";    break;
            case Level::Warning: levelAsString = "Warning"; break;
            case Level::Error:   levelAsString = "Error";   break;
            case Level::Fatal:   levelAsString = "Fatal";   break;
        }
        if( levelAsString == "Error" || levelAsString == "Fatal" ) {
            errorOccurred = true;
        }
        preparedOutput.emplace_back( levelAsString + ": " + diagnostic.text );
        if( diagnostic.firstPos < diagnostic.lastPos ) {
            auto errorShow = sourceCmdLine + "\n";
            errorShow.append( diagnostic.firstPos, ' ' );
            errorShow.append( diagnostic.lastPos - diagnostic.firstPos, '~' );
            preparedOutput.emplace_back( errorShow );
        }
    }
    if( errorOccurred ) {
        preparedOutput.emplace_back( "\n--help\t\t\tDisplay all available options." );
    }
    return preparedOutput;
}
