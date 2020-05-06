#ifndef __LINTERCOMBINE_H__
#define __LINTERCOMBINE_H__

#include "Factory.h"
#include "LinterWrapperItf.h"

#include <memory>
#include <vector>

namespace LintCombine {
    class LinterCombine : public LinterWrapperItf {
    public:
        LinterCombine( int argc, char ** argv );

        void callLinter() const override;

        int waitLinter() const override;

        CallTotals updatedYaml() const override;

        std::shared_ptr < LinterWrapperItf > linterAt( int pos );

        int numLinters();

    private:
        std::vector < std::shared_ptr < LinterWrapperItf > > linters;
        Factory::Service service;
    };
}
#endif //__LINTERCOMBINE_H__
