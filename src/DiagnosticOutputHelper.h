#pragma once

#include "LinterItf.h"

namespace LintCombine {
    class DiagnosticOutputHelper {
    public:
        void setCmdLine( const StringVector & cmdLineVal ) {
            m_cmdLine = cmdLineVal;
        }

        bool printDiagnostics( std::vector< Diagnostic > && diagnostics ) const;

        bool printDiagnostics( std::vector< Diagnostic > & diagnostics ) const;

    private:
        bool printDiagnosticsBase( std::vector< Diagnostic > & diagnostics ) const;

        static std::string howToPrintHelpStr();

        static std::string productInfoStr();

        static std::string helpStr();

        StringVector prepareOutput( std::vector< Diagnostic > & diagnostics ) const;

        StringVector m_cmdLine;
    };
}
