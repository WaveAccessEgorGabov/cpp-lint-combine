#pragma once

#include "PrepareCmdLineReSharper.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <algorithm>

namespace LintCombine {

    class PrepareCmdLineFactory {
    public:
        class PrepareCmdLineOnError final : public PrepareCmdLineItf {

        public:
            PrepareCmdLineOnError( std::string && textVal, const Level levelVal,
                                   const unsigned firstPosVal, const unsigned lastPosVal )
                : m_diagnostics{ Diagnostic( std::move( textVal ), levelVal,
                                             firstPosVal, lastPosVal ) } {}

            stringVector transform( stringVector commandLine ) override {
                for( const auto & it : m_diagnostics ) {
                    if( it.level == Level::Error || it.level == Level::Fatal ) {
                        return stringVector();
                    }
                }
                return commandLine;
            }

            std::vector< Diagnostic > diagnostics() override {
                return m_diagnostics;
            }

        private:
            std::vector< Diagnostic > m_diagnostics;
        };

        static PrepareCmdLineItf * createInstancePrepareCmdLine( stringVector & cmdLine );

        // TODO: fix hyphens number in moveCommandLineToSTLContainer() ?
    private:
        static void fixHyphensInCmdLine( stringVector & cmdLine );
    };
}
