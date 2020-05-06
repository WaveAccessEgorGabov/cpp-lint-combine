#ifndef LINTERWRAPPER_FACTORY_H
#define LINTERWRAPPER_FACTORY_H

#include "LinterWrapperItf.h"

#include <boost/asio.hpp>
#include <memory>
#include <vector>

namespace LintCombine {
    class Factory {
    public:
        Factory() = delete;

        Factory( const Factory & ) = delete;

        Factory( Factory && ) = delete;

        class Service {
        public:
            boost::asio::io_service & getIO_Service();

        private:
            boost::asio::io_service ios;
        };

        static Factory & getInstance();

        std::vector < std::shared_ptr < LinterWrapperItf>> createLinters( int argc, char ** argv, Service service );

        Service getService();
    };
}


#endif //LINTERWRAPPER_FACTORY_H
