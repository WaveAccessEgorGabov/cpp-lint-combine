#include "ClangTidyWrapper.h"

LintCombine::ClangTidyWrapper::ClangTidyWrapper( const stringVector & cmdLine,
                                                 LinterFactoryBase::Services & service )
    : LinterBase( cmdLine, service, /*name=*/"clang-tidy" ) {}

void LintCombine::ClangTidyWrapper::updateYamlData( YAML::Node & yamlNode ) const {
    addDocLink( yamlNode );
}

void LintCombine::ClangTidyWrapper::addDocLink( YAML::Node & yamlNode ) {
    for( auto diagnostic : yamlNode["Diagnostics"] ) {
        std::ostringstream docLink;
        docLink << "https://clang.llvm.org/extra/clang-tidy/checks/" << diagnostic["DiagnosticName"] << ".html";
        diagnostic["DocumentationLink"] = docLink.str();
    }
}
