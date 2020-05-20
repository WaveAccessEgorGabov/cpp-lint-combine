#include "LinterBase.h"

#include <fstream>
#include <iostream>

void LintCombine::LinterBase::readFromPipeToStream( boost::process::async_pipe& pipe, std::ostream& stream ) {
    pipe.async_read_some(boost::process::buffer(buffer), [ & ]( boost::system::error_code ec, size_t size ) {
            stream.write(buffer.data(), size);
            if( !ec )
                readFromPipeToStream(pipe, stream);
        });
}

LintCombine::LinterBase::LinterBase( FactoryBase::Services & service )
        : service( service ), stdoutPipe( service.getIO_Service() ), stderrPipe( service.getIO_Service() ) {
}

void LintCombine::LinterBase::callLinter() {
    linterProcess = boost::process::child( name,
                                           boost::process::std_out > stdoutPipe,
                                           boost::process::std_err > stderrPipe );
    readFromPipeToStream(stdoutPipe, std::cout);
    readFromPipeToStream(stderrPipe, std::cerr);
}

int LintCombine::LinterBase::waitLinter() {
    linterProcess.wait();
    return linterProcess.exit_code();
}

LintCombine::CallTotals LintCombine::LinterBase::updateYaml() const {
    YAML::Node yamlNode;
    try {
        yamlNode = YAML::LoadFile( yamlPath );
    }
    catch( const YAML::BadFile & ex ) {
        std::cerr << "Exception while load .yaml; what(): " << ex.what() << std::endl;
        return CallTotals( /*success=*/ 0, /*fail=*/ 1 );
    }

    updateYamlAction( yamlNode );

    try {
        std::ofstream yamlWithDocLinkFile( yamlPath );
        yamlWithDocLinkFile << yamlNode;
    }
    catch( const std::ios_base::failure & ex ) {
        std::cerr << "Exception while updating .yaml " << "what(): " << ex.what() << std::endl;
        return CallTotals( /*success=*/ 0, /*fail=*/ 1 );
    }

    return CallTotals( /*success=*/ 1, /*fail=*/ 0 );
}

const std::string & LintCombine::LinterBase::getName() const {
    return name;
}

const std::string & LintCombine::LinterBase::getOptions() const {
    return options;
}

std::string LintCombine::LinterBase::getYamlPath() const {
    return yamlPath;
}
