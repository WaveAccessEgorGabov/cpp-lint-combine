#pragma once

#include "LinterItf.h"

namespace LintCombine {

    class DiagnosticWorker {

    public:
        explicit DiagnosticWorker( const stringVector & cmdLineVal,
                                   const bool isCmdLineEmptyVal )
            : m_cmdLine( cmdLineVal ), m_isCmdLineEmpty( isCmdLineEmptyVal ) {}

        bool printDiagnostics( std::vector< Diagnostic > & diagnostics ) const;

    private:
        static std::string howToPrintHelpStr();

        static std::string productInfoStr();

        static std::string helpStr();

        void sortDiagnostics( std::vector< Diagnostic > & diagnostics ) const;

        stringVector prepareOutput( std::vector< Diagnostic > & diagnostics ) const;

        stringVector m_cmdLine;

        bool m_isCmdLineEmpty;
    };
}
