#include "ClazyWrapper.h"

#include <sstream>

void ClazyWrapper::addDocLinkToYaml( const YAML::Node & yamlNode ) const {
    for( auto it : yamlNode[ "Diagnostics" ] ) {
        std::ostringstream documentationLink;
        std::ostringstream diagnosticName;
        diagnosticName << it[ "DiagnosticName" ];
        documentationLink << "https://github.com/KDE/clazy/blob/master/docs/checks/README-";
        // substr() from 6 to size() for skipping "clazy-" in DiagnosticName
        documentationLink << diagnosticName.str().substr( 6, diagnosticName.str().size() ) << ".md";
        it[ "Documentation link" ] = documentationLink.str();
    }
}
