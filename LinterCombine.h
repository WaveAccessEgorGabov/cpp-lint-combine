#ifndef __LINTERCOMBINE_H__
#define __LINTERCOMBINE_H__

#include "FactoryBase.h"
#include "UsualFactory.h"
#include "LinterItf.h"

#include <memory>
#include <vector>

namespace LintCombine {
    class LinterCombine : public LinterItf {
    public:
        LinterCombine( int argc, char ** argv, FactoryBase & factory = UsualFactory::getInstance() );

        void callLinter() const override;

        int waitLinter() const override;

        CallTotals updatedYaml() const override;

        std::shared_ptr < LinterItf > linterAt( int pos );

        int numLinters();

    private:

        std::vector < char ** > splitCommandLineByLinters( int, char ** );

        std::vector < std::pair < std::string, char **>> getLinterNameWithOptions( std::vector < char ** > );

        std::vector < std::shared_ptr < LinterItf > > linters;
        FactoryBase::Services service;
    };
}
#endif //__LINTERCOMBINE_H__
