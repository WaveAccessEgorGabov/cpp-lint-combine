#ifndef __LINTERWRAPPER_FACTORY_H__
#define __LINTERWRAPPER_FACTORY_H__

#include "LinterItf.h"

#include <boost/asio.hpp>
#include <memory>
#include <vector>

namespace LintCombine {
    class FactoryBase {
    public:
        class Services {
        public:
            boost::asio::io_service & getIO_Service();

        private:
            boost::asio::io_service ios;
        };

        FactoryBase() = delete;

        FactoryBase( const FactoryBase & ) = delete;

        FactoryBase( FactoryBase && ) = delete;

        std::vector < std::shared_ptr < LinterItf > > getLinter( int argc, char ** argv );

        Services & getService();

    protected:
        Services services;

        virtual std::vector < std::shared_ptr < LinterItf>>
        createLinter( std::vector < std::pair < std::string, char ** >> lintersAndTheirOptions ) = 0;

    private:
        std::vector < std::pair < std::string, char ** >> splitCommandLineByLinter( int argc, char ** argv );
    };
}


#endif //__LINTERWRAPPER_FACTORY_H__
