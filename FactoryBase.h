#ifndef __LINTERWRAPPER_FACTORY_H__
#define __LINTERWRAPPER_FACTORY_H__

#include    "LinterItf.h"

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

        Services & getService();

    protected:
        Services services;

        virtual std::shared_ptr < LinterItf >
        createLinter( std::vector < std::pair < std::string, char ** >> lintersAndTheirOptions ) = 0;
    };
}


#endif //__LINTERWRAPPER_FACTORY_H__
