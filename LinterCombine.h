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

        std::string getYamlPath() const final;

        CallTotals updateYaml() const final;

        std::shared_ptr < LinterItf > linterAt( int pos ) const;

        size_t numLinters() const noexcept;

        bool printTextIfRequested() const;

    private:
        static char ** vectorStringToCharPP( const std::vector < std::string > & stringVector );

        std::vector < std::vector < std::string > > splitCommandLineByLinters( int argc, char ** argv );

        std::string combineYaml( std::vector < std::string > yamlPaths );

        bool m_helpIsRequested = false;
        std::vector < std::shared_ptr < LinterItf > > m_linters;
        FactoryBase::Services & service;
    };
}
#endif //__LINTERCOMBINE_H__
