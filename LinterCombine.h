#ifndef __LINTERCOMBINE_H__
#define __LINTERCOMBINE_H__

#include "UsualFactory.h"
#include "LinterItf.h"

#include <memory>
#include <vector>

namespace LintCombine {
    class LinterCombine final : public LinterItf {
    public:
        explicit LinterCombine( int argc, char ** argv, FactoryBase & factory = UsualFactory::getInstance() );

        void callLinter() final;

        int waitLinter() final;

        CallTotals updateYaml() const final;

        std::shared_ptr < LinterItf > linterAt( int pos ) const;

        int numLinters() const noexcept;

    private:

        std::vector < std::vector < std::string > > splitCommandLineByLinters( int argc, char ** argv );

        std::vector < std::shared_ptr < LinterItf > > linters;
        FactoryBase::Services & service;
    };
}
#endif //__LINTERCOMBINE_H__
