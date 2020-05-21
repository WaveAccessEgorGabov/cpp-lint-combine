#include "LinterBase.h"

#include <fstream>
#include <iostream>

void LintCombine::LinterBase::callLinter() {
    linterProcess = boost::process::child( name,
                                           boost::process::std_out > stdoutPipe,
                                           boost::process::std_err > stderrPipe );
    readFromPipeToStream( stdoutPipe, std::cout );
    readFromPipeToStream( stderrPipe, std::cerr );
}

int LintCombine::LinterBase::waitLinter() {
    linterProcess.wait();
    return linterProcess.exit_code();
}

LintCombine::CallTotals LintCombine::LinterBase::updateYaml() const {
    if( yamlPath.empty() ) {
        return CallTotals( /*successNum=*/ 0, /*failNum=*/ 1 );
    }
    YAML::Node yamlNode;
    try {
        yamlNode = YAML::LoadFile( yamlPath );
    }
    catch( const YAML::BadFile & ex ) {
        std::cerr << "YAML::BadFile exception while load yaml-file. What(): " << ex.what() << std::endl;
        return CallTotals( /*successNum=*/ 0, /*failNum=*/ 1 );
    }
    catch( const std::exception & ex ) {
        std::cerr << "Exception while load yaml-file. What(): " << ex.what() << std::endl;
        return CallTotals( /*successNum=*/ 0, /*failNum=*/ 1 );
    }

    updateYamlAction( yamlNode );

    try {
        std::ofstream yamlWithDocLinkFile( yamlPath );
        yamlWithDocLinkFile << yamlNode;
    }
    catch( const std::ios_base::failure & ex ) {
        std::cerr << "std::ios_base::failure exception while updating yaml-file. What(): " << ex.what() << std::endl;
        return CallTotals( /*successNum=*/ 0, /*failNum=*/ 1 );
    }
    catch( const std::exception & ex ) {
        std::cerr << "Exception while updating yaml-file. What(): " << ex.what() << std::endl;
        return CallTotals( /*successNum=*/ 0, /*failNum=*/ 1 );
    }

    return CallTotals( /*successNum=*/ 1, /*failNum=*/ 0 );
}

const std::string & LintCombine::LinterBase::getName() const {
    return name;
}

const std::string & LintCombine::LinterBase::getOptions() const {
    return options;
}

const std::string & LintCombine::LinterBase::getYamlPath() {
    return yamlPath;
}

LintCombine::LinterBase::LinterBase( FactoryBase::Services & service )
        : stdoutPipe( service.getIO_Service() ), stderrPipe( service.getIO_Service() ) {
}

void LintCombine::LinterBase::readFromPipeToStream( boost::process::async_pipe & pipe, std::ostream & outputStream ) {
    pipe.async_read_some( boost::process::buffer( buffer ), [ & ]( boost::system::error_code ec, size_t size ) {
        outputStream.write( buffer.data(), size );
        if( !ec )
            readFromPipeToStream( pipe, outputStream );
    } );
}
