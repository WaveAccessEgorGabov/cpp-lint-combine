#include <string>
#include <filesystem>
#include <process.h>

int wmain( int argc, wchar_t ** argv ) {
    std::wstring childProcessCmdLine =
        L"\"" + std::filesystem::path( argv[0] ).parent_path().wstring() +
        L"/clang-tidy/cpp-lint-combine-msvc.cmd\"";
    for( int i = 1; i < argc; childProcessCmdLine += L" " + std::wstring( argv[i++] ) );
    return _wsystem( childProcessCmdLine.c_str() );
}
