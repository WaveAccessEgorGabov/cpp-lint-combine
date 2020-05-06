#ifndef __LINTERWRAPPERITF_H__
#define __LINTERWRAPPERITF_H__

#include "CallTotals.h"

namespace LintCombine {
    class LinterWrapperItf {
    public:
        virtual void callLinter() const = 0;

        virtual int waitLinter() const = 0;

        virtual CallTotals updatedYaml() const = 0;
    };
}

#endif //__LINTERWRAPPERITF_H__
