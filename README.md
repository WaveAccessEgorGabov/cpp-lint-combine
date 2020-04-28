# cpp-lint-combine

# Current linters
## [clang-tidy](https://clang.llvm.org/extra/clang-tidy/)
You must create compile_commands.json file if you want to analyzing your project's files.

**Create compile_commands.json. Ubuntu. Cmake**  
1) Go to project directory, where files for analyzing is located
2) mkdir _build && cd _build
3) cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release/Debug ..

**Create compile_commands.json. Windows. Cmake**  
On windows cmake project generator from Visual Studio can't create compile_commands.json, so you must using another cmake project generator, for example, Unix Makefiles
1) Go to project directory, where files for analyzing is located
2) mkdir _build && cd _build
3) cmake -G "Unix Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release/Debug ..

## [clazy](https://github.com/KDE/clazy)
Qmake can't generated compile_commands.json, so you have to using [Bear](https://github.com/rizsotto/Bear). Bear is a tool that generates a compilation database for clang tooling.

**Create compile_commands.json. Ubuntu. Qmake**  
1) Go to project directory, where files for analyzing is located
2) mkdir _build && cd _build
3) qmake .. && bear make

# Build lintWrapper

## For build you have to install several tools:

**YAML parser**

Program is using yaml-parser. Install yaml-parser:
1) Go to project directory
2) ```git clone https://github.com/jbeder/yaml-cpp.git```

**Boost Libraries**

Program is using Boost libraries. You can install Boost libraries from https://www.boost.org/

**Clang-Tidy**

Install on Ubuntu: sudo apt-get install -y clang-tidy

**Clazy-standalone**

You can find clazy-standalone in QtCreator. Or install from [github](https://github.com/KDE/clazy)

**Build programm**

1) Go to project directory
2) mkdir _build && cd _build
4) cmake -DCMAKE_BUILD_TYPE=Release/Debug ..
5) cmake --build .
