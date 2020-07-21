#include "ClazyWrapper.h"

LintCombine::ClazyWrapper::ClazyWrapper( stringVector && commandLineSTL,
                                         LinterFactoryBase::Services & service )
        // TODO: "--standalone" for clazy installed binary
        : LinterBase( commandLineSTL, service, "clazy-standalone" ) {
}

void LintCombine::ClazyWrapper::updateYamlAction( const YAML::Node & yamlNode ) const {
    addDocumentationLink(yamlNode);
}

void LintCombine::ClazyWrapper::addDocumentationLink(const YAML::Node & yamlNode ) {
    for( auto it : yamlNode[ "Diagnostics" ] ) {
        std::ostringstream documentationLink;
        std::ostringstream diagnosticName;
        diagnosticName << it[ "DiagnosticName" ];
        if( diagnosticName.str().find ( "clazy-" ) == 0 ) {
            documentationLink << "https://github.com/KDE/clazy/blob/master/docs/checks/README-"
                << diagnosticName.str().substr ( std::string ( "clazy-" ).size (),
                                                  diagnosticName.str ().size () ) << ".md";
            it[ "DocumentationLink" ] = documentationLink.str ();
        }
        else {
            it[ "DocumentationLink" ] = std::string();
        }
    }
}
