#include "ClazyWrapper.h"

void ClazyWrapper::addDocLinkToYaml( const YAML::Node & yamlNode ) {
    for ( auto it : yamlNode[ "Diagnostics" ] ) {
        std::ostringstream documentationLink;
        std::ostringstream tempOss;
        tempOss << it[ "DiagnosticName" ];
        documentationLink << "https://github.com/KDE/clazy/blob/master/docs/checks/README-";
        // substr() from 6 to size() for skipping "clazy-" in DiagnosticName
        documentationLink << tempOss.str().substr( 6, tempOss.str().size() ) << ".md";
        it[ "Documentation link" ] = documentationLink.str();
    }
}
