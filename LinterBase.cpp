#include "LinterBase.h"

#include <iostream>

LintCombine::LinterBase::LinterBase( FactoryBase::Services & service )
        : service( service ), stdoutPipe( service.getIO_Service() ), stderrPipe( service.getIO_Service() ) {
    readFromPipeToStdout =
            [ this, buffer = std::array < char, 64 > {} ]( boost::process::async_pipe & pipe ) mutable {
                pipe.async_read_some( boost::process::buffer( buffer ),
                                      [ & ]( boost::system::error_code ec, size_t size ) {
                                          std::cout.write( buffer.data(), size );
                                          if( !ec )
                                              readFromPipeToStdout( pipe );
                                      } );
            };

    readFromPipeToStderr =
            [ this, buffer = std::array < char, 64 > {} ]( boost::process::async_pipe & pipe ) mutable {
                pipe.async_read_some( boost::process::buffer( buffer ),
                                      [ & ]( boost::system::error_code ec, size_t size ) {
                                          std::cerr.write( buffer.data(), size );
                                          if( !ec )
                                              readFromPipeToStderr( pipe );
                                      } );
            };
}

// ToDo Problem with lambda parameters, should make one lambda for both streams
void LintCombine::LinterBase::callLinter() {
    linterProcess = boost::process::child( name,
                                           boost::process::std_out > stdoutPipe, boost::process::std_err > stderrPipe );
    readFromPipeToStdout( stdoutPipe );
    readFromPipeToStderr( stderrPipe );
}

int LintCombine::LinterBase::waitLinter() {
    linterProcess.wait();
    return linterProcess.exit_code();
}

LintCombine::CallTotals LintCombine::LinterBase::updateYaml() {
    YAML::Node yamlNode;
    try {
        yamlNode = YAML::LoadFile( yamlPath );
    }
    catch( const YAML::BadFile & ex ) {
        std::cerr << "Exception while load .yaml; what(): " << ex.what() << std::endl;
        return CallTotals( /*success=*/ 0, /*fail=*/ 1 );
    }
    // specific linter must return bool
    updateYamlAction( yamlNode );

    try {
        std::ofstream yamlWithDocLinkFile( yamlPath );
        yamlWithDocLinkFile << yamlNode;
    }
    catch( const std::ios_base::failure & ex ) {
        std::cerr << "Exception while writing updated .yaml " << "what(): " << ex.what() << std::endl;
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

const std::string & LintCombine::LinterBase::getYamlPath() const {
    return yamlPath;
}
