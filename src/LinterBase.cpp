#include "LinterBase.h"

#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>

std::vector<LintCombine::Diagnostic> LintCombine::LinterBase::diagnostics() {
    return m_diagnostics;
}

void LintCombine::LinterBase::callLinter() {
    linterProcess = boost::process::child( name + " --export-fixes=" + yamlPath + " " + options,
                                           boost::process::std_out > stdoutPipe,
                                           boost::process::std_err > stderrPipe );
    readFromPipeToStream( stdoutPipe, std::cout );
    readFromPipeToStream( stderrPipe, std::cerr );
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
    catch( const std::exception & error ) {
        m_diagnostics.emplace_back( Diagnostic( Level::Error, error.what(),
                                    "LinterBase", 1, 0 ) );
        return CallTotals( /*successNum=*/ 0, /*failNum=*/ 1 );
    }

    updateYamlAction( yamlNode );

    try {
        std::ofstream yamlWithDocLinkFile( yamlPath );
        yamlWithDocLinkFile << yamlNode;
    }
    catch( const std::exception & error ) {
        m_diagnostics.emplace_back( Diagnostic( Level::Error, error.what(),
                                    "LinterBase", 1, 0 ) );
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

LintCombine::LinterBase::LinterBase( LinterFactoryBase::Services & service )
    : stdoutPipe( service.getIO_Service() ),
    stderrPipe( service.getIO_Service() ) {}

LintCombine::LinterBase::LinterBase( const stringVector & cmdLine,
                                     LinterFactoryBase::Services & service,
                                     std::string && nameVal )
    : name( nameVal ), stdoutPipe( service.getIO_Service() ),
    stderrPipe( service.getIO_Service() ) {
    parseCommandLine( cmdLine );
}

void LintCombine::LinterBase::parseCommandLine( const stringVector & cmdLine ) {
    boost::program_options::options_description optDesc;
    optDesc.add_options()
        ( "export-fixes",
          boost::program_options::value< std::string >( &yamlPath )
          -> default_value( CURRENT_BINARY_DIR + name + "-Diagnostics.yaml" ) );
    boost::program_options::variables_map vm;
    try {
        const boost::program_options::parsed_options parsed =
            boost::program_options::command_line_parser( cmdLine )
            .style( boost::program_options::command_line_style::default_style |
                    boost::program_options::command_line_style::allow_long_disguise )
            .options( optDesc ).allow_unregistered().run();
        store( parsed, vm );
        notify( vm );
        const stringVector linterOptionsVec =
            collect_unrecognized( parsed.options,
            boost::program_options::include_positional );
        for( const auto & it : linterOptionsVec ) {
            options.append( it + " " );
        }
    }
    catch( const std::exception & error ) {
        m_diagnostics.emplace_back( Diagnostic( Level::Warning, error.what(),
                                    name.c_str(), 1, 0 ) );
        yamlPath = CURRENT_BINARY_DIR + name + "-Diagnostics.yaml";
        return;
    }

    const auto yamlFilename =
        boost::filesystem::path( yamlPath ).filename().string();
    if( !boost::filesystem::portable_name( yamlFilename ) ) {
        m_diagnostics.emplace_back(
            Diagnostic( Level::Warning,
            "Incorrect linter's yaml name: \"" + yamlFilename +
            "\"", name.c_str(), 1, 0 ) );
        yamlPath = CURRENT_BINARY_DIR + name + "-Diagnostics.yaml";
    }
}

void LintCombine::LinterBase::readFromPipeToStream( boost::process::async_pipe & pipe,
                                                    std::ostream & outputStream ) {
    pipe.async_read_some( boost::process::buffer( m_buffer ),
                          [&]( boost::system::error_code ec, size_t size ) {
                              outputStream.write( m_buffer.data(), size );
                              if( !ec )
                                  readFromPipeToStream( pipe, outputStream );
    } );
}
