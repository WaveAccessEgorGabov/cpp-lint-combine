#pragma once

#include "PrepareInputsItf.h"
#include "IdeBehaviorItf.h"

#include <boost/program_options.hpp>
#include <memory>

namespace LintCombine {
    class IdeTraitsFactory {
    public:
        IdeTraitsFactory( StringVector & cmdLine );

        class PrepareInputsOnError final : public PrepareInputsItf {
        public:
            PrepareInputsOnError( const Level levelVal,
                const std::string & textVal, const std::string & originVal,
                const unsigned firstPosVal, const unsigned lastPosVal )
                : m_diagnostics{ Diagnostic( levelVal, textVal, originVal,
                                             firstPosVal, lastPosVal ) } {}

            StringVector transformCmdLine( const StringVector & commandLine ) override {
                for( const auto & diagnostic : m_diagnostics ) {
                    if( diagnostic.level == Level::Error || diagnostic.level == Level::Fatal ) {
                        return {};
                    }
                }
                return commandLine;
            }

            void transformFiles() override {}

            std::vector< Diagnostic > diagnostics() const override {
                return m_diagnostics;
            }

        private:
            std::vector< Diagnostic > m_diagnostics;
        };

        std::unique_ptr< PrepareInputsItf > getPrepareInputsInstance();

        std::unique_ptr< IdeBehaviorItf > getIdeBehaviorInstance();

    private:
        std::string m_ideName;
        StringVector & m_cmdLine;
    };
}
