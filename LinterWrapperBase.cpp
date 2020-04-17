#include "LinterWrapperBase.h"

#include <fstream>
#include <iostream>
#include <boost/process.hpp>

int LinterWrapperBase::callLinter( bool isNeedHelp ) const {
    if( linterName.empty() ) {
        std::cerr << "Expected: linter name is not empty" << std::endl;
        return 1;
    }
#ifdef WIN32
    std::string linterExecutableCommand( linterName + ".exe " + linterOptions );
#elif __linux__
    std::string linterExecutableCommand = linterName + " " + linterOptions;
#endif
    if( !yamlFilePath.empty() ) {
        linterExecutableCommand.append( " --export-fixes=" + yamlFilePath );
    }
    if( isNeedHelp ) {
        linterExecutableCommand.append( "--help" );
        std::cout << "Information about chosen linter: " << std::endl;
    }
    try {
        boost::process::child linterProcess( linterExecutableCommand );
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
        std::ofstream yamlWithDocLinkFile( CURRENT_SOURCE_DIR"/linterYamlWithDocLink.yaml" );
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
