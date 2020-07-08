#pragma once

#include "PrepareCmdLineReSharper.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <algorithm>

namespace LintCombine {

    class PrepareCmdLineFactory {
    public:
        class PrepareCmdLineOnError final : public PrepareCmdLineItf {

        public:
            PrepareCmdLineOnError( std::string && textVal, const Level levelVal,
                                   const unsigned firstPosVal, const unsigned lastPosVal )
                : m_diagnostics{ Diagnostic( std::move( textVal ), levelVal,
                                             firstPosVal, lastPosVal ) } {}

            stringVector transform( stringVector commandLine ) override {
                for( const auto & it : m_diagnostics ) {
                    if( it.level == Level::Error || it.level == Level::Fatal ) {
                        return stringVector();
                    }
                }
                return commandLine;
            }

            std::vector< Diagnostic > diagnostics() override {
                return m_diagnostics;
            }

        private:
            std::vector< Diagnostic > m_diagnostics;
        };

        static PrepareCmdLineItf * createInstancePrepareCmdLine( stringVector & cmdLine ) {
            if( cmdLine.empty() ) {
                return new PrepareCmdLineOnError( "Command line is empty",
                                             Level::Error, 1, 0 );
            }
            fixHyphensInCmdLine( cmdLine );
            std::string ideName;
            boost::program_options::options_description programDesc;
            programDesc.add_options()
                ( "ide-profile",
                  boost::program_options::value < std::string >( &ideName ) );
            boost::program_options::variables_map vm;
            try {
                store( boost::program_options::command_line_parser( cmdLine ).
                       options( programDesc ).allow_unregistered().run(), vm );
                notify( vm );
            }
            catch( const std::exception & ex ) {
                return new PrepareCmdLineOnError( std::move( ex.what() ),
                                                  Level::Error, 1, 0 );
            }
            cmdLine.erase( std::remove_if( std::begin( cmdLine ), std::end( cmdLine ),
                           [ideName]( const std::string & str ) -> bool {
                               return str.find( "--ide-profile" ) == 0 ||
                                   str.compare( ideName ) == 0;
                           } ), std::end( cmdLine ) );
            if( ideName.empty() ) {
                return new PrepareCmdLineOnError( "Options were passed verbatim",
                                              Level::Info, 1, 0 );
            }
            const auto ideNameCopy = ideName;
            boost::algorithm::to_lower( ideName );
            if( ideName == "resharper" ) {
                return new PrepareCmdLineReSharper();
            }
            // TODO: find position of incorrect IDE
            return new PrepareCmdLineOnError( "\"" + ideNameCopy +
                                              "\" isn't supported by cpp-lint-combine",
                                              Level::Error, 1, 0 );
        }
        // TODO: fix hyphens number in moveCommandLineToSTLContainer() ?
    private:
        static void fixHyphensInCmdLine( stringVector & cmdLine ) {
            for( auto & it : cmdLine ) {
                if( it.find( "--" ) != 0 && it.find( '-' ) == 0 ) {
                    if( it.find( '=' ) != std::string::npos ) {
                        // -param=value -> --param=value
                        if( it.find( '=' ) != std::string( "-p" ).size() ) {
                            it.insert( 0, "-" );
                        }
                    }
                    // -param value -> --param value
                    else if( it.size() > std::string( "-p" ).size() ) {
                        it.insert( 0, "-" );
                    }
                }
                if( it.find( "--" ) == 0 ) {
                    if( it.find( '=' ) != std::string::npos ) {
                        // --p=value -> -p=value
                        if( it.find( '=' ) == std::string( "--p" ).size() ) {
                            it.erase( it.begin() );
                        }
                    }
                    // --p value -> -p value
                    else if( it.size() == std::string( "--p" ).size() ) {
                        it.erase( it.begin() );
                    }
                }
            }
        }
    };
}
