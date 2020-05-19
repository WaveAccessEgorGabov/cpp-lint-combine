#ifndef __LINTERWRAPPERITF_H__
#define __LINTERWRAPPERITF_H__

#include "CallTotals.h"

namespace LintCombine {
    class LinterItf {
    public:
	    virtual ~LinterItf() = default;

	    virtual void callLinter() = 0;

        virtual int waitLinter() = 0;

        virtual CallTotals updateYaml() const = 0;
    };
}

#endif //__LINTERWRAPPERITF_H__
