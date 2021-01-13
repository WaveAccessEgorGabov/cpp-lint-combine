#include "LinterBase.h"
#include "LintCombineUtils.h"
#include "LintCombineException.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/program_options.hpp>

#include <filesystem>
#include <fstream>

LintCombine::LinterBase::~LinterBase() {
    std::error_code errorCode;
    std::filesystem::remove( yamlPath, errorCode );
}

std::vector< LintCombine::Diagnostic > LintCombine::LinterBase::diagnostics() const {
    return m_diagnostics;
}

void LintCombine::LinterBase::callLinter( const std::unique_ptr< IdeBehaviorItf > & ideBehavior ) {
    try {
        m_linterProcess = m_linterBehavior->createProcess(
            ideBehavior->doesMergeStdoutAndStderr(), name, yamlPath,
            m_options, m_stdoutPipe, m_stderrPipe );
    }
    catch( const std::exception & ex ) {
        m_diagnostics.emplace_back( Level::Error, ex.what(), "LinterBase", 1, 0 );
        m_linterProcess.terminate();
        return;
    }
    m_convertLinterOutput = ideBehavior->doesConvertLinterOutput();
    m_convertLinterOutputEncoding = ideBehavior->doesConvertLinterOutputEncoding();
    readFromPipeToStream( m_stdoutPipe, std::cout );
    if( !ideBehavior->doesMergeStdoutAndStderr() ) {
        readFromPipeToStream( m_stderrPipe, std::cerr );
    }
}

int LintCombine::LinterBase::waitLinter() {
    if( m_linterProcess.valid() ) { // linter doesn't terminate
        m_linterProcess.wait();
        return m_linterProcess.exit_code();
    }
    return 1;
}

LintCombine::CallTotals LintCombine::LinterBase::updateYaml() {
    if( yamlPath.empty() ) {
        return { /*successNum=*/ 0, /*failNum=*/ 0 };
    }
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

LintCombine::CallTotals LintCombine::LinterBase::getYamlPath( std::string & yamlPathOut ) {
    if( yamlPath.empty() ) {
        yamlPathOut.clear();
        return { /*successNum=*/1, /*failNum=*/0 };
    }
    if( !std::filesystem::exists( yamlPath ) ) {
        yamlPathOut = yamlPath;
        return { /*successNum=*/ 0, /*failNum=*/ 1 };
    }
    yamlPathOut = yamlPath;
    return { /*successNum=*/ 1, /*failNum=*/ 0 };
}

LintCombine::LinterBase::LinterBase( LinterFactoryBase::Services & service,
                                     std::unique_ptr< LinterBehaviorItf > && linterBehaviorVal )
    : m_stdoutPipe( service.getIOService() ),
      m_stderrPipe( service.getIOService() ),
      m_linterBehavior( std::move( linterBehaviorVal ) ) {}

LintCombine::LinterBase::LinterBase( const StringVector & cmdLine,
                                     LinterFactoryBase::Services & service,
                                     const std::string & nameVal,
                                     std::unique_ptr< LinterBehaviorItf > && linterBehaviorVal )
    : name( nameVal ), m_stdoutPipe( service.getIOService() ),
      m_stderrPipe( service.getIOService() ),
      m_linterBehavior( std::move( linterBehaviorVal ) ) {
    parseCmdLine( cmdLine );
}

void LintCombine::LinterBase::parseCmdLine( const StringVector & cmdLine ) {
    boost::program_options::options_description optDesc;
    optDesc.add_options()(
        "export-fixes",
        boost::program_options::value< std::string >( &yamlPath )->implicit_value( {} ) );
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

    checkIsOptionsValueInit( boost::algorithm::join( cmdLine, " " ),
                             m_diagnostics, "export-fixes", yamlPath,
                             name.c_str(), "Path to " + name + "'s YAML-file is not set" );
    if( !yamlPath.empty() && !isFileCreatable( yamlPath ) ) {
        m_diagnostics.emplace_back(
            Level::Error, name + "'s YAML-file \"" + yamlPath + "\" is not creatable",
            name.c_str(), 1, 0 );
        throw Exception( m_diagnostics );
    }
}

void LintCombine::LinterBase::readFromPipeToStream( boost::process::async_pipe & pipe,
                                                    std::ostream & outputStream ) {
    pipe.async_read_some( boost::process::buffer( m_readPart ),
                          [&]( boost::system::error_code ec, std::streamsize readPartSize ) {
        auto & currentWorkBuffer = m_streamBufferMap[ &outputStream ];
        const std::string receivedMes(
            m_readPart.data(), 0, static_cast< std::string::size_type >( readPartSize ) );
        auto bufferWithReadPart = currentWorkBuffer + receivedMes;
        std::streamsize convertedCharsNum = -1;
        if( m_convertLinterOutput ) {
            convertedCharsNum = m_linterBehavior->convertLinterOutput( bufferWithReadPart );
        }
        if( m_convertLinterOutputEncoding ) {
            bufferWithReadPart.replace(
                0, convertedCharsNum,
                convertStringEncodingFromUTF8ToCP437( bufferWithReadPart.substr( 0, convertedCharsNum ) ) );
        }
        if( convertedCharsNum < 0 ) {
            outputStream.write( bufferWithReadPart.c_str(), bufferWithReadPart.size() );
            currentWorkBuffer.clear();
        }
        if( convertedCharsNum >= 0 ) {
            outputStream.write( bufferWithReadPart.c_str(), convertedCharsNum );
            currentWorkBuffer = bufferWithReadPart.substr( convertedCharsNum );
        }
        if( !ec )
            readFromPipeToStream( pipe, outputStream );
        else
            outputStream << currentWorkBuffer;
    } );
}
