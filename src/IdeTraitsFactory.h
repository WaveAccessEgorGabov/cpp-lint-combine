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
            virtual bool getDoesLinterExitCodeTolerant() = 0;
            virtual ~IdeBehaviorItf() = default;
        };

        class IdeBehaviorBase final : public IdeBehaviorItf {

        public:
            IdeBehaviorBase( const bool doesAddLinkVal,
                const bool linterExitCodeTolerantVal )
                : m_doesAddLink( doesAddLinkVal ),
                  m_linterExitCodeTolerant (linterExitCodeTolerantVal) {}

            bool getDoesAddLink() override { return m_doesAddLink; }

            bool getDoesLinterExitCodeTolerant() override { return m_linterExitCodeTolerant; }

        private:
            bool m_doesAddLink;
            bool m_linterExitCodeTolerant;
        };


        class PrepareInputsOnError final : public PrepareInputsItf {

        public:
            PrepareInputsOnError( const Level levelVal,
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

        std::shared_ptr< PrepareInputsItf > getPrepareInputsInstance( stringVector & cmdLine );

        std::shared_ptr < IdeBehaviorItf > getIdeBehaviorInstance();

    private:
        std::string m_ideName;
    };
}
