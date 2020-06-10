#include "ClazyWrapper.h"

LintCombine::ClazyWrapper::ClazyWrapper( stringVector && commandLineSTL,
                                         FactoryBase::Services & service )
        : LinterBase( commandLineSTL, service ) {
    name = "clazy-standalone";
}

void LintCombine::ClazyWrapper::updateYamlAction( const YAML::Node & yamlNode ) const {
    addDocLinkToYaml( yamlNode );
}

void LintCombine::ClazyWrapper::addDocLinkToYaml( const YAML::Node & yamlNode ) {
    for( auto it : yamlNode[ "Diagnostics" ] ) {
        std::ostringstream documentationLink;
        std::ostringstream diagnosticName;
        diagnosticName << it[ "DiagnosticName" ];
        documentationLink << "https://github.com/KDE/clazy/blob/master/docs/checks/README-"
                          << diagnosticName.str().substr( std::string( "clazy-" ).size(), diagnosticName.str().size() )
                          << ".md";
        it[ "Documentation link" ] = documentationLink.str();
    }
}