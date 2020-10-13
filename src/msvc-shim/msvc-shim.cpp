#include <string>
#include <filesystem>

int main( int argc, char ** argv ) {
    std::string childProcessCmdLine =
        PATH_TO_COMBINES_BOOTSTRAP " " +
        std::filesystem::path( argv[0] ).parent_path().string() + "/clang-tidy/clang-tidy.exe";
    for( int i = 1; i < argc; childProcessCmdLine += " " + std::string( argv[i++] ) );
    return std::system( childProcessCmdLine.c_str() );
}
