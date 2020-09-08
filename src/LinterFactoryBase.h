#pragma once

#include "LinterItf.h"

#include <boost/asio.hpp>
#include <memory>
#include <vector>

namespace LintCombine {
    class LinterFactoryBase {

    public:
        virtual ~LinterFactoryBase() = default;

        class Services {

        public:
            boost::asio::io_service & getIOService() {
                return m_ios;
            }

        private:
            boost::asio::io_service m_ios;
        };

        Services & getServices() {
            return services;
        }

        virtual std::unique_ptr< LinterItf > createLinter( const StringVector & cmdLine ) = 0;

    protected:
        Services services;
    };
}
