#include <cstdlib>
#include <string>

int main( int argc, char ** argv ) {
    std::string cmdLine = PATH_TO_COMBINES_BOOTSTRAP;
    for( int i = 1; i < argc; cmdLine += " ", cmdLine += argv[i++] );
    return std::system( cmdLine.c_str() );
}
