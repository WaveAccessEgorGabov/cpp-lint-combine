#include "ClangTidyWrapper.h"

LintCombine::ClangTidyWrapper::ClangTidyWrapper( const stringVector & cmdLine,
                                                 LinterFactoryBase::Services & service )
    : LinterBase( cmdLine, service, "clang-tidy" ) {}

void LintCombine::ClangTidyWrapper::updateYamlAction( const YAML::Node & yamlNode ) const {
    addDocLink( yamlNode );
}

void LintCombine::ClangTidyWrapper::addDocLink( const YAML::Node & yamlNode ) {
    for( auto it : yamlNode["Diagnostics"] ) {
        std::ostringstream docLink;
        docLink << "https://clang.llvm.org/extra/clang-tidy/checks/" << it["DiagnosticName"] << ".html";
        it["DocumentationLink"] = docLink.str();
    }
}

