#include "ClazyWrapper.h"

LintCombine::ClazyWrapper::ClazyWrapper( const StringVector & cmdLine,
    LinterFactoryBase::Services & service )
    // TODO: "--standalone" for clazy installed binary (in Linux)
    : LinterBase( cmdLine, service, /*name=*/"clazy-standalone" ) {}

void LintCombine::ClazyWrapper::updateYamlData( YAML::Node & yamlNode ) const {
    addDocLink( yamlNode );
}

void LintCombine::ClazyWrapper::addDocLink( YAML::Node & yamlNode ) {
    for( auto diagnostic : yamlNode["Diagnostics"] ) {
        std::ostringstream docLink;
        std::ostringstream diagnosticName;
        diagnosticName << diagnostic["DiagnosticName"];
        if( diagnosticName.str().find( "clazy-" ) == 0 ) {
            docLink << "https://github.com/KDE/clazy/blob/master/docs/checks/README-"
                << diagnosticName.str().substr( std::string( "clazy-" ).size(),
                diagnosticName.str().size() ) << ".md";
            diagnostic["DocumentationLink"] = docLink.str();
        }
        else {
            diagnostic["DocumentationLink"] = std::string();
        }
    }
}
