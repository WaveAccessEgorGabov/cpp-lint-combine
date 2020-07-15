#pragma once

#include "LinterItf.h"

#include <ostream>

namespace LintCombine {

    class DiagnosticWorker {

    public:
        explicit DiagnosticWorker( const stringVector & cmdLineVal,
                                   const bool isCmdLineEmptyVal )
            : cmdLine( cmdLineVal ), isCmdLineEmpty( isCmdLineEmptyVal ) {}

        bool printDiagnostics( const std::vector< Diagnostic > & diagnostics ) const;

    private:
        static std::string getHowToPrintHelpStr();

        static std::string getProductInfoStr();

        static std::string getHelpStr();

        stringVector prepareOutput( const std::vector< Diagnostic > & diagnostics ) const;

        stringVector cmdLine;

        bool isCmdLineEmpty;
    };
}
