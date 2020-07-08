#include "ClangTidyWrapper.h"

LintCombine::ClangTidyWrapper::ClangTidyWrapper( stringVector && commandLine,
                                                 LinterFactoryBase::Services & service )
        : LinterBase( commandLine, service ) {
    name = "clang-tidy";
}

void LintCombine::ClangTidyWrapper::updateYamlAction( const YAML::Node & yamlNode ) const {
    addDocLinkToYaml( yamlNode );
}

void LintCombine::ClangTidyWrapper::addDocLinkToYaml( const YAML::Node & yamlNode ) {
    for( auto it : yamlNode[ "Diagnostics" ] ) {
        std::ostringstream documentationLink;
        documentationLink << "https://clang.llvm.org/extra/clang-tidy/checks/" << it[ "DiagnosticName" ] << ".html";
        it[ "Documentation link" ] = documentationLink.str();
    }
}
