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
            boost::asio::io_service & getIO_Service() {
                return ios;
            }

        private:
            boost::asio::io_service ios;
        };

        Services & getService() {
            return services;
        }

        virtual std::shared_ptr < LinterItf > createLinter( int argc, char ** argv ) = 0;

    protected:

        Services services;
    };
}


#endif //__LINTERWRAPPER_FACTORY_H__
