#include "LinterWrapperBase.h"

#include <fstream>
#include <iostream>
#include <boost/process.hpp>
#include <boost/asio.hpp>

int LinterWrapperBase::callLinter( bool isNeedHelp ) const {
    if( linterName.empty() ) {
        std::cerr << "Expected: linter name is not empty" << std::endl;
        return 1;
    }

    std::string linterExecutableCommand = linterName + " " + linterOptions;
    if( !yamlFilePath.empty() ) {
        linterExecutableCommand.append( " --export-fixes=" + yamlFilePath );
    }
    if( isNeedHelp ) {
        linterExecutableCommand.append( "--help" );
        std::cout << "Information about chosen linter: " << std::endl;
    }
    try {
        boost::asio::io_service ios;
        boost::process::async_pipe stdoutPipe ( ios );
        boost::process::async_pipe stderrPipe ( ios );

        boost::process::child linterProcess(
                linterExecutableCommand,
                boost::process::std_out > stdoutPipe,
                boost::process::std_err > stderrPipe, ios );

        std::function < void() > asyncWriteToStdout = [ &, buffer = std::array < char, 64 > {} ]() mutable {
            stdoutPipe.async_read_some( boost::process::buffer( buffer ), [ & ]( boost::system::error_code ec, size_t size ) {
                std::cout.write( buffer.data(), size );
                if( !ec )
                    asyncWriteToStdout();
            } );
        };

        std::function < void() > asyncWriteToStderr = [ &, buffer = std::array < char, 64 > {} ]() mutable {
            stderrPipe.async_read_some( boost::process::buffer( buffer ), [ & ]( boost::system::error_code ec, size_t size ) {
                std::cerr.write( buffer.data(), size );
                if( !ec )
                    asyncWriteToStderr();
            } );
        };

        asyncWriteToStdout();
        asyncWriteToStderr();
        ios.run();
        linterProcess.wait();
        return linterProcess.exit_code();
    }
    catch( const boost::process::process_error & ex ) {
        std::cerr << "Exception while run linter; what(): " << ex.what() << std::endl;
        return 1;
    }
    catch( ... ) {
        std::cerr << "Exception while run linter" << std::endl;
        return 1;
    }
}

bool LinterWrapperBase::createUpdatedYaml() const {
    if( yamlFilePath.empty() ) {
        std::cerr << ".yaml file is empty" << std::endl;
        return false;
    }

    YAML::Node yamlNode;
    try {
        yamlNode = YAML::LoadFile( yamlFilePath );
    }
    catch( const YAML::BadFile & ex ) {
        std::cerr << "Exception while load .yaml; what(): " << ex.what() << std::endl;
        return false;
    }
    catch( ... ) {
        std::cerr << "Exception while load .yaml" << std::endl;
        return false;
    }

    addDocLinkToYaml( yamlNode );

    try {
        std::ofstream yamlWithDocLinkFile( CURRENT_SOURCE_DIR"linterYamlWithDocLink.yaml" );
        yamlWithDocLinkFile << yamlNode;
        return true;
    }
    catch( const std::ios_base::failure & ex ) {
        std::cerr << "Exception while writing updated .yaml " << "what(): " << ex.what() << std::endl;
        return false;
    }
    catch( ... ) {
        std::cerr << "Exception while writing updated .yaml " << std::endl;
        return false;
    }
}


const std::string & LinterWrapperBase::getLinterName() const {
    return linterName;
}

const std::string & LinterWrapperBase::getLinterOptions() const {
    return linterOptions;
}

const std::string & LinterWrapperBase::getYamlFilePath() const {
    return yamlFilePath;
}
