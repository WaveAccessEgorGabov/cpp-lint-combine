#pragma once

#include "LinterItf.h"

namespace LintCombine {
    class OutputHelper {
    public:
        void setCmdLine( const StringVector & cmdLineVal );

        static void printHelp();

        static void printHelpOption();

        static void printProductInfo();

        void printDiagnostics( const std::vector< Diagnostic > & diagnostics ) const;

    private:
        StringVector prepareOutput( const std::vector< Diagnostic > & diagnostics ) const;

        StringVector m_cmdLine;
    };
}
