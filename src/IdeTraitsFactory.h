#pragma once

#include "PrepareInputsItf.h"

#include <boost/program_options.hpp>
#include <memory>

namespace LintCombine {

    class IdeTraitsFactory {

    public:
        class IdeBehaviorItf {

        public:
            virtual bool mayYamlContainsDocLink() const = 0;
            virtual bool isLinterExitCodeTolerant() const = 0;
            virtual ~IdeBehaviorItf() = default;
        };

        class IdeBehaviorBase final : public IdeBehaviorItf {

        public:
            IdeBehaviorBase( const bool yamlContainsDocLinkVal,
                const bool linterExitCodeTolerantVal )
                : m_yamlContainsDocLink( yamlContainsDocLinkVal ),
                m_linterExitCodeTolerant( linterExitCodeTolerantVal ) {}

            bool mayYamlContainsDocLink() const override { return m_yamlContainsDocLink; }

            bool isLinterExitCodeTolerant() const override { return m_linterExitCodeTolerant; }

        private:
            bool m_yamlContainsDocLink;
            bool m_linterExitCodeTolerant;
        };

        class PrepareInputsOnError final : public PrepareInputsItf {

        public:
            PrepareInputsOnError( const Level levelVal,
                const std::string & textVal, const std::string & originVal,
                const unsigned firstPosVal, const unsigned lastPosVal )
                : m_diagnostics{ Diagnostic( levelVal, textVal, originVal,
                                             firstPosVal, lastPosVal ) } {}

            stringVector transformCmdLine( const stringVector & commandLine ) override {
                for( const auto & it : m_diagnostics ) {
                    if( it.level == Level::Error || it.level == Level::Fatal ) {
                        return {};
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

        std::unique_ptr< PrepareInputsItf > getPrepareInputsInstance( stringVector & cmdLine );

        std::unique_ptr< IdeBehaviorItf > getIdeBehaviorInstance();

    private:
        std::string m_ideName;
    };
}
