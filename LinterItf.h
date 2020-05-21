#pragma once

#include "CallTotals.h"
#include <string>

namespace LintCombine {
    class LinterItf {
    public:
	    virtual ~LinterItf() = default;

	    virtual void callLinter() = 0;

        virtual int waitLinter() = 0;

        virtual CallTotals updateYaml() const = 0;

        virtual const std::string & getYamlPath() = 0;
    };
}

