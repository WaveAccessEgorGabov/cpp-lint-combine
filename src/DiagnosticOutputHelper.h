#pragma once

#include "LinterItf.h"

namespace LintCombine {
    class DiagnosticOutputHelper {
    public:
        explicit DiagnosticOutputHelper( const StringVector & cmdLineVal,
                                         const bool isCmdLineEmptyVal )
            : m_cmdLine( cmdLineVal ), m_isCmdLineEmpty( isCmdLineEmptyVal ) {}

        bool printDiagnostics( std::vector< Diagnostic > && diagnostics ) const;

        bool printDiagnostics( std::vector< Diagnostic > & diagnostics ) const;

    private:
        bool printDiagnosticsBase( std::vector< Diagnostic > & diagnostics ) const;

        static std::string howToPrintHelpStr();

        static std::string productInfoStr();

        static std::string helpStr();

        StringVector prepareOutput( std::vector< Diagnostic > & diagnostics ) const;

        StringVector m_cmdLine;

        bool m_isCmdLineEmpty;
    };
}
