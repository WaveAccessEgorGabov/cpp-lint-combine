#include "ClazyWrapper.h"

LintCombine::ClazyWrapper::ClazyWrapper( const stringVector & cmdLine,
                                         LinterFactoryBase::Services & service )
    // TODO: "--standalone" for clazy installed binary (in Linux)
    : LinterBase( cmdLine, service, "clazy-standalone" ) {}

void LintCombine::ClazyWrapper::updateYamlAction( const YAML::Node & yamlNode ) const {
    addDocLink( yamlNode );
}

void LintCombine::ClazyWrapper::addDocLink( const YAML::Node & yamlNode ) {
    for( auto it : yamlNode["Diagnostics"] ) {
        std::ostringstream docLink;
        std::ostringstream diagnosticName;
        diagnosticName << it["DiagnosticName"];
        if( diagnosticName.str().find( "clazy-" ) == 0 ) {
            docLink << "https://github.com/KDE/clazy/blob/master/docs/checks/README-"
                << diagnosticName.str().substr( std::string( "clazy-" ).size(),
                                                diagnosticName.str().size() ) << ".md";
            it["DocumentationLink"] = docLink.str();
        }
        else {
            it["DocumentationLink"] = std::string();
        }
    }
}
