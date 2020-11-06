#include <string>
#include <filesystem>

int main( int argc, char ** argv ) {
    std::string childProcessCmdLine =
        "\"" + std::filesystem::path( argv[0] ).parent_path().string() +
        "/clang-tidy/cpp-lint-combine-msvc.cmd\"";
    for( int i = 1; i < argc; childProcessCmdLine += " " + std::string( argv[i++] ) );
    return std::system( childProcessCmdLine.c_str() );
}
