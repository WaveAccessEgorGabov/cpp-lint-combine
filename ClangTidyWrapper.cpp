#include "ClangTidyWrapper.h"

void ClangTidyWrapper::addDocLinkToYaml( const YAML::Node & yamlNode ) const {
    for( auto it : yamlNode[ "Diagnostics" ] ) {
        std::ostringstream documentationLink;
        documentationLink << "https://clang.llvm.org/extra/clang-tidy/checks/" << it[ "DiagnosticName" ] << ".html";
        it[ "Documentation link" ] = documentationLink.str();
    }
}
