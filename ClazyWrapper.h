#ifndef __CLANGTIDYWRAPPER_H__
#define __CLANGTIDYWRAPPER_H__

#include "Factory.h"
#include "LinterWrapperBase.h"

#include <boost/asio.hpp>
#include <boost/process.hpp>

namespace LintCombine {
    class ClazyWrapper : public LinterWrapperBase {
    public:
        ClazyWrapper( int argc, char ** argv, Factory::Service & service );

    private:
        void updateYamlAction( const YAML::Node & yamlNode ) const override;

        void addDocLinkToYaml( const YAML::Node & yamlNode ) const;

        void parseCommandLine( int argc, char ** argv ) override;

        Factory::Service service;
        boost::process::child linterProcess;
        boost::process::async_pipe stdoutPipe;
        boost::process::async_pipe stderrPipe;
        std::function <void (boost::process::async_pipe & )> readFromPipe;
    };
}
#endif //__CLANGTIDYWRAPPER_H__
