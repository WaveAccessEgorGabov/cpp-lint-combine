#include "DiagnosticWorker.h"
#include PATH_TO_VERSION_RESOURCE

#include <boost/algorithm/string/join.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>

std::string LintCombine::DiagnosticWorker::helpStr() {
    boost::program_options::options_description optDesc( /*line_length=*/100 );
    optDesc.add_options()
        ( "help", "Print this message." )
        ( "ide-profile", "Choose IDE: ReSharper C++, CLion. "
        "By default options will pass verbatim." )
        ( "result-yaml",
        "Path to YAML with diagnostics from all linters." )
        ( "sub-linter",
        "Linter to use. You can use this param several times to set"
        " several linters. Supported linters are: clang-tidy, clazy." )
        ( "clazy-checks",
        "Comma-separated list of clazy checks. Default is level1." )
        ( "clang-extra-args",
        "Additional argument to append to the compiler command line." );
    std::string str;
    str.append( productInfoStr() );
    str.append( "Usage:\n" );
    str.append( boost::lexical_cast< std::string >( optDesc ) );
    return str;
}

std::string LintCombine::DiagnosticWorker::howToPrintHelpStr() {
    boost::program_options::options_description optDesc;
    optDesc.add_options()
        ( "help", "Display all available options." );
    return boost::lexical_cast< std::string >( optDesc );
}

std::string LintCombine::DiagnosticWorker::productInfoStr() {
    return PRODUCTNAME_STR " " PRODUCTVERSION_STR "\n\n";
}

bool LintCombine::DiagnosticWorker::printDiagnostics(
    const std::vector< Diagnostic > & diagnostics ) const {
    if( m_isCmdLineEmpty ) {
        std::cout << productInfoStr();
        std::cout << howToPrintHelpStr();
        return true;
    }

    for( const auto & cmdLineEl : m_cmdLine ) {
        if( cmdLineEl == "--help" ) {
            std::cout << helpStr();
            return true;
        }
    }

    for( const auto & diagnostic : prepareOutput( diagnostics ) ) {
        std::cerr << diagnostic << std::endl;
    }

    return false;
}

LintCombine::stringVector
LintCombine::DiagnosticWorker::prepareOutput(
    const std::vector< Diagnostic > & diagnostics ) const {
    const auto sourceCmdLine = boost::algorithm::join( m_cmdLine, " " );
    stringVector preparedOutput;
    auto errorOccurred = false;
    for( const auto & diagnostic : diagnostics ) {
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
        preparedOutput.emplace_back( "\n--help\t\t\tDisplay all available options.\n" );
    }
    return preparedOutput;
}
