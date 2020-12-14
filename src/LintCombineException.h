#pragma once

#include "Diagnostic.h"

#include <vector>

namespace LintCombine {
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