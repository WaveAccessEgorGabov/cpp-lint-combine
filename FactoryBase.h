#ifndef __LINTERWRAPPER_FACTORY_H__
#define __LINTERWRAPPER_FACTORY_H__

#include "LinterItf.h"

#include <boost/asio.hpp>
#include <memory>
#include <vector>

namespace LintCombine {
    class FactoryBase {
    public:
	    virtual ~FactoryBase() = default;

	    class Services {
        public:
            boost::asio::io_service & getIO_Service() {
                return m_ios;
            }

        private:
            boost::asio::io_service m_ios;
        };

        Services & getService() {
            return m_services;
        }

        virtual std::shared_ptr < LinterItf > createLinter( int argc, char ** argv ) = 0;

    protected:

        Services m_services;
    };
}


#endif //__LINTERWRAPPER_FACTORY_H__
