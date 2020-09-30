#include "LinterBase.h"
#include "LintCombineUtils.h"

#include <boost/program_options.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

std::vector< LintCombine::Diagnostic > LintCombine::LinterBase::diagnostics() const {
    return m_diagnostics;
}

void LintCombine::LinterBase::callLinter() {
    try{
        m_linterProcess = boost::process::child(
            name + " --export-fixes=" + yamlPath + " " + m_options,
            boost::process::std_out > m_stdoutPipe,
            boost::process::std_err > m_stderrPipe );
    }
    catch( const std::exception & ex ) {
        m_diagnostics.emplace_back( Level::Error, ex.what(), "LinterBase", 1, 0 );
        m_linterProcess.terminate();
        return;
    }
    readFromPipeToStream( m_stdoutPipe, std::cout );
    readFromPipeToStream( m_stderrPipe, std::cerr );
}

int LintCombine::LinterBase::waitLinter() {
    if( m_linterProcess.valid() ) { // linter doesn't terminate
        m_linterProcess.wait();
        return m_linterProcess.exit_code();
    }
    return 1;
}

LintCombine::CallTotals LintCombine::LinterBase::updateYaml() {
    if( !std::filesystem::exists( yamlPath ) ) {
        m_diagnostics.emplace_back(
            Level::Warning, name + "'s YAML-file doesn't exist", "LinterBase", 1, 0 );
        return { /*successNum=*/ 0, /*failNum=*/ 1 };
    }

    const std::ifstream filePathToYaml( yamlPath );
    if( filePathToYaml.fail() ) {
        m_diagnostics.emplace_back(
            Level::Error,
            "An error occurred while opening \"" + yamlPath + "\" for reading", "LinterBase", 1, 0 );
        return { /*successNum=*/ 0, /*failNum=*/ 1 };
    }

    YAML::Node yamlNode = YAML::LoadFile(yamlPath);
    updateYamlData( yamlNode );

    std::ofstream yamlWithDocLinkFile( yamlPath );
    if( yamlWithDocLinkFile.fail() ) {
        m_diagnostics.emplace_back(
            Level::Error,
            "An error occurred while opening \"" + yamlPath + "\" for writing", "LinterBase", 1, 0 );
            return { /*successNum=*/ 0, /*failNum=*/ 1 };
    }
    yamlWithDocLinkFile << yamlNode;
    return { /*successNum=*/ 1, /*failNum=*/ 0 };
}

std::string LintCombine::LinterBase::getName() const {
    return name;
}

std::string LintCombine::LinterBase::getOptions() const {
    return m_options;
}

std::string LintCombine::LinterBase::getYamlPath() {
    return yamlPath;
}

LintCombine::LinterBase::LinterBase( LinterFactoryBase::Services & service )
    : m_stdoutPipe( service.getIOService() ),
      m_stderrPipe( service.getIOService() ) {}

LintCombine::LinterBase::LinterBase( const StringVector & cmdLine,
                                     LinterFactoryBase::Services & service,
                                     const std::string & nameVal )
    : name( nameVal ), m_stdoutPipe( service.getIOService() ),
      m_stderrPipe( service.getIOService() ) {
    parseCmdLine( cmdLine );
}

void LintCombine::LinterBase::parseCmdLine( const StringVector & cmdLine ) {
    boost::program_options::options_description optDesc;
    optDesc.add_options()(
        "export-fixes", boost::program_options::value< std::string >( &yamlPath ) );
    boost::program_options::variables_map vm;
    try {
        const boost::program_options::parsed_options parsed =
            boost::program_options::command_line_parser( cmdLine )
            .style( boost::program_options::command_line_style::default_style |
                    boost::program_options::command_line_style::allow_long_disguise )
            .options( optDesc ).allow_unregistered().run();
        store( parsed, vm );
        notify( vm );
        const StringVector linterOptions =
            collect_unrecognized( parsed.options,
                                  boost::program_options::include_positional );
        for( const auto & option : linterOptions ) {
            m_options.append( option + " " );
        }
    }
    catch( const std::exception & ex ) {
        m_diagnostics.emplace_back( Level::Error, ex.what(), name.c_str(), 1, 0 );
        throw Exception( m_diagnostics );
    }

    if( yamlPath.empty() ) {
        m_diagnostics.emplace_back(
            Level::Error, "Path to linter's YAML-file is not set", name.c_str(), 1, 0 );
        throw Exception( m_diagnostics );
    }

    if( !isFileCreatable( yamlPath ) ) {
        m_diagnostics.emplace_back(
            Level::Error, "Linter's YAML-file \"" + yamlPath + "\" is not creatable",
            name.c_str(), 1, 0 );
        throw Exception( m_diagnostics );
    }
}

void LintCombine::LinterBase::readFromPipeToStream( boost::process::async_pipe & pipe,
                                                    std::ostream & outputStream ) {
    pipe.async_read_some( boost::process::buffer( m_buffer ),
                          [&]( boost::system::error_code ec, std::streamsize size ) {
        outputStream.write( m_buffer.data(), size );
        if( !ec )
            readFromPipeToStream( pipe, outputStream );
    } );
}
