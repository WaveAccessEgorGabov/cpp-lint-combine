#ifndef __LINTERCOMBINE_H__
#define __LINTERCOMBINE_H__

#include "FactoryBase.h"
#include "StandardFactory.h"
#include "LinterItf.h"

#include <memory>
#include <vector>

namespace LintCombine {
    class LinterCombine : public LinterItf {
    public:
        LinterCombine( int argc, char ** argv, FactoryBase& factory = StandardFactory::getInstance() );

        void callLinter() const override;

        int waitLinter() const override;

        CallTotals updatedYaml() const override;

        std::shared_ptr < LinterItf > linterAt( int pos );

        int numLinters();

    private:
        std::vector < std::shared_ptr < LinterItf > > linters;
        FactoryBase::Services service;
    };
}
#endif //__LINTERCOMBINE_H__
