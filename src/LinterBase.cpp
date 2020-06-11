#include "LinterBase.h"

#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>

void LintCombine::LinterBase::callLinter() {
    linterProcess = boost::process::child( name + " " + options + "--export-fixes=" + yamlPath,
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

LintCombine::LinterBase::LinterBase( const stringVector & commandLine, FactoryBase::Services & service )
        : stdoutPipe( service.getIO_Service() ), stderrPipe( service.getIO_Service() ) {
    parseCommandLine( commandLine );
    checkYamlPathForCorrectness();
}

void LintCombine::LinterBase::parseCommandLine( const stringVector & commandLine ) {
    boost::program_options::options_description programOptions;
    programOptions.add_options()
            ( "export-fixes", boost::program_options::value < std::string >( & yamlPath ) );
    boost::program_options::variables_map vm;
    const boost::program_options::parsed_options parsed =
            boost::program_options::command_line_parser( commandLine ).
                    options( programOptions ).style( boost::program_options::command_line_style::default_style |
                        boost::program_options::command_line_style::allow_long_disguise).allow_unregistered().run();
    boost::program_options::store( parsed , vm );
    std::vector < std::string > linterOptionsVec
            = boost::program_options::collect_unrecognized( parsed.options,
                                                            boost::program_options::include_positional );
    boost::program_options::notify( vm );

    for( const auto & it : linterOptionsVec ) {
        options.append( it + " " );
    }
}

void LintCombine::LinterBase::checkYamlPathForCorrectness() {
    std::string yamlFileName = boost::filesystem::path( yamlPath ).filename().string();
    if( !boost::filesystem::portable_name( yamlFileName ) ) {
        std::cerr << "\"" << yamlFileName << "\" is incorrect file name! (linter's yaml-path incorrect)" << std::endl;
        yamlPath = std::string();
    }
}

void LintCombine::LinterBase::readFromPipeToStream( boost::process::async_pipe & pipe, std::ostream & outputStream ) {
    pipe.async_read_some( boost::process::buffer( buffer ), [ & ]( boost::system::error_code ec, size_t size ) {
        outputStream.write( buffer.data(), size );
        if( !ec )
            readFromPipeToStream( pipe, outputStream );
    } );
}
