#pragma once

#include "PrepareInputsItf.h"

#include <boost/program_options.hpp>
#include <algorithm>
#include <memory>

namespace LintCombine {

    class IdeTraitsFactory {

    public:
        class IdeBehaviorItf {

        public:
            virtual bool getDoesAddLink() = 0;
        };

        class IdeBehaviorBase final : public IdeBehaviorItf {

        public:
            IdeBehaviorBase( const bool doesAddLinkVal )
                : doesAddLink( doesAddLinkVal ) {}

            bool getDoesAddLink() override { return doesAddLink; }

        private:
            bool doesAddLink;
        };


        class PrepareCmdLineOnError final : public PrepareInputsItf {

        public:
            PrepareCmdLineOnError( const Level levelVal,
                                   std::string && textVal,
                                   std::string && originVal,
                                   const unsigned firstPosVal,
                                   const unsigned lastPosVal )
                : m_diagnostics{ Diagnostic( levelVal, std::move( textVal ),
                                             std::move( originVal ),
                                             firstPosVal, lastPosVal ) } {}

            stringVector transformCmdLine( stringVector commandLine ) override {
                for( const auto & it : m_diagnostics ) {
                    if( it.level == Level::Error || it.level == Level::Fatal ) {
                        return stringVector();
                    }
                }
                return commandLine;
            }

            void transformFiles() override {}

            std::vector< Diagnostic > diagnostics() override {
                return m_diagnostics;
            }

        private:
            std::vector< Diagnostic > m_diagnostics;
        };

        std::shared_ptr< PrepareInputsItf > getPrepareCmdLineInstance( stringVector & cmdLine );

        std::shared_ptr < IdeBehaviorItf > getIdeBehaviorInstance();

    private:
        std::string ideName;
    };
}
