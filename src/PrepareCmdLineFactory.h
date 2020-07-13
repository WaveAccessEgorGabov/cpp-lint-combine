#pragma once

#include "PrepareCmdLineItf.h"

#include <boost/program_options.hpp>
#include <algorithm>

namespace LintCombine {

    class PrepareCmdLineFactory {
    public:
        class PrepareCmdLineOnError final : public PrepareCmdLineItf {

        public:
            PrepareCmdLineOnError( std::string && textVal, std::string && originVal,
                                   const Level levelVal, const unsigned firstPosVal,
                                   const unsigned lastPosVal )
                : m_diagnostics{ Diagnostic( std::move( textVal ), std::move( originVal ),
                                             levelVal, firstPosVal, lastPosVal ) } {}

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
    };
}
