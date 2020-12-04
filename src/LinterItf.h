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
}
