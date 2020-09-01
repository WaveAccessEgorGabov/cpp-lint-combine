#include "DiagnosticWorker.h"
#include PATH_TO_VERSION_RESOURCE

#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>

std::string LintCombine::DiagnosticWorker::getHelpStr() {
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
    str.append( getProductInfoStr() );
    str.append( "Usage:\n" );
    str.append( boost::lexical_cast< std::string >( optDesc ) );
    return str;
}

std::string LintCombine::DiagnosticWorker::getHowToPrintHelpStr() {
    boost::program_options::options_description optDesc;
    optDesc.add_options()
        ( "help", "Display all available options." );
    return boost::lexical_cast< std::string >( optDesc );
}

std::string LintCombine::DiagnosticWorker::getProductInfoStr() {
    return PRODUCTNAME_STR " " PRODUCTVERSION_STR "\n\n";
}

bool LintCombine::DiagnosticWorker::printDiagnostics(
    const std::vector< Diagnostic > & diagnostics ) const {
    if( m_isCmdLineEmpty ) {
        std::cout << getProductInfoStr();
        std::cout << getHowToPrintHelpStr();
        return true;
    }

    for( const auto & it : m_cmdLine ) {
        if( it == "--help" ) {
            std::cout << getHelpStr();
            return true;
        }
    }

    for( const auto & it : prepareOutput( diagnostics ) ) {
        std::cerr << it << std::endl;
    }

    return false;
}

LintCombine::stringVector
LintCombine::DiagnosticWorker::prepareOutput(
    const std::vector< Diagnostic > & diagnostics ) const {
    const auto sourceCL = boost::algorithm::join( m_cmdLine, " " );
    stringVector preparedOutput;
    auto isErrorOccur = false;
    for( const auto & it : diagnostics ) {
        std::string levelAsString;
        switch( it.level ) {
            case Level::Trace:   levelAsString = "Trace";   break;
            case Level::Debug:   levelAsString = "Debug";   break;
            case Level::Info:    levelAsString = "Info";    break;
            case Level::Warning: levelAsString = "Warning"; break;
            case Level::Error:   levelAsString = "Error";   break;
            case Level::Fatal:   levelAsString = "Fatal";   break;
        }
        if( levelAsString == "Error" || levelAsString == "Fatal" ) {
            isErrorOccur = true;
        }
        preparedOutput.emplace_back( levelAsString + ": " + it.text );
        if( it.firstPos < it.lastPos ) {
            auto errorShow = sourceCL + "\n";
            errorShow.append( it.firstPos, ' ' );
            errorShow.append( it.lastPos - it.firstPos, '~' );
            preparedOutput.emplace_back( errorShow );
        }
    }
    if( isErrorOccur ) {
        preparedOutput.emplace_back( "\n--help\t\t\tDisplay all available options.\n" );
    }
    return preparedOutput;
}

