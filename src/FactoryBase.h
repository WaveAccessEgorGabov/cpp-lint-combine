#pragma once

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

        Services & getServices() {
            return services;
        }

        virtual std::shared_ptr < LinterItf > createLinter( stringVectorConstRef commandLineSTL ) = 0;

    protected:

        Services services;
    };
}
