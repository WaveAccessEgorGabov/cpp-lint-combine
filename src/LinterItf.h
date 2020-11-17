#pragma once

#include "CallTotals.h"
#include "Diagnostic.h"
#include "IdeBehaviorItf.h"

#include <vector>
#include <memory>

namespace LintCombine {
    using StringVector = std::vector< std::string >;

    struct LinterItf {
        virtual ~LinterItf() = default;

        virtual void callLinter( const std::unique_ptr< IdeBehaviorItf > & ideBehavior ) = 0;

        virtual int waitLinter() = 0;

        virtual CallTotals updateYaml() = 0;

        virtual CallTotals getYamlPath( std::string & yamlPathOut ) = 0;

        virtual std::vector< Diagnostic > diagnostics() const = 0;
    };

    class Exception final : public std::exception {
    public:
        Exception( const Diagnostic & diagnostic )
            : m_diagnostics{ diagnostic } {}

        Exception( const std::vector< Diagnostic > & diagnostics )
            : m_diagnostics( diagnostics ) {}

        std::vector< Diagnostic > diagnostics() const {
            return m_diagnostics;
        }

    private:
        std::vector< Diagnostic > m_diagnostics;
    };
}
