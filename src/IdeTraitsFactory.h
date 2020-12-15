#pragma once

#include "PrepareInputsItf.h"
#include "IdeBehaviorItf.h"

#include <boost/program_options.hpp>
#include <memory>

namespace LintCombine {
    class IdeTraitsFactory {
    public:
        // Constructor modifies the command line arguments by
        // removing "--ide-profile" option and the option's value
        IdeTraitsFactory( StringVector & cmdLineVal );

        std::unique_ptr< PrepareInputsItf > getPrepareInputsInstance() const;

        std::unique_ptr< IdeBehaviorItf > getIdeBehaviorInstance() const;

        std::vector< Diagnostic > diagnostics() const;

    private:
        std::string m_ideName;
        std::vector< Diagnostic > m_diagnostics;
    };
}
