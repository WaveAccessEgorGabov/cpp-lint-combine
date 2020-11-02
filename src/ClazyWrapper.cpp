#include "ClazyWrapper.h"

LintCombine::ClazyWrapper::ClazyWrapper( const StringVector & cmdLine,
                                         LinterFactoryBase::Services & service,
                                         std::unique_ptr < LinterBehaviorItf > && linterBehaviorVal )
    // TODO: "--standalone" for clazy installed binary (in Linux)
    : LinterBase( cmdLine, service, /*name=*/"clazy-standalone", std::move( linterBehaviorVal ) ) {}

void LintCombine::ClazyWrapper::updateYamlData( YAML::Node & yamlNode ) const {
    addDocLink( yamlNode );
}

void LintCombine::ClazyWrapper::addDocLink( YAML::Node & yamlNode ) {
    for( auto /* shallow copy */ diagnostic : yamlNode["Diagnostics"] ) {
        auto diagnosticName = diagnostic["DiagnosticName"].as< std::string >( "" );
        constexpr char prefix[] = "clazy-";
        if( diagnosticName.find( prefix ) == 0 ) {
            diagnostic["DocumentationLink"] =
                "https://github.com/KDE/clazy/blob/master/docs/checks/README-" +
                diagnosticName.substr( std::size( prefix ) - 1 ) + ".md";
        }
        else {
            diagnostic["DocumentationLink"] = std::string();
        }
    }
}
