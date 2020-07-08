# Lint-combine

### 1. Overview
Lint-combine is a tool that lets you combine several [linters](https://en.wikipedia.org/wiki/Lint_(software)) and use them all in an IDE that supports only some (e.g. only one).
#### 1.1 What is a linter
**lint**, or a **linter**, is a tool that analyzes source code to flag programming errors, bugs, stylistic errors, and suspicious constructs. 
#### 1.2 Using lint-combine
You can use lint-combine as a **command line tool**, or in [**ReSharper C++**](https://www.jetbrains.com/resharper-cpp/) — Visual Studio extension. 

### 2. Get the required tools 
- **Boost** — minimum required version is **1.69.0**, but you cannot use version **1.72.0**, because it contains [an error in the boost::process](https://github.com/boostorg/process/issues/116). 
Download and build (at least Datetime, Regex, ProgramOptions, and FileSystem) from [sources](https://www.boost.org/users/download/) or download and install [prebuilt Windows binaries](https://sourceforge.net/projects/boost/files/boost-binaries/) into “`<boost-dir>`” (substitute a path of your choosing).
- **CMake** — minimum required version is **3.14**. Install from [here](https://cmake.org/download/).
- **Git** — Install from [here](https://git-scm.com/download). Ensure that Git's `bin` directory is listed in `PATH` earlier than any other directory containing `sh.exe` (or any other file invoked by the `sh` command).
- **Visual Studio** — required version is **2017+**. You can install Visual Studio 2019 from [here](https://visualstudio.microsoft.com/downloads/).

### 3. Build lint-combine  
#### Windows
```sh
git clone https://github.com/WaveAccessEgorGabov/cpp-lint-combine.git <lint-combine-source-dir>
cd <lint-combine-source-dir>
git checkout develop
git submodule update --init --recursive yaml-cpp
cmake -S <lint-combine-source-dir> -B <lint-combine-build-dir> -A x64 -DBOOST_ROOT=<boost-dir>
cmake --build <lint-combine-build-dir> --config Release
```

#### Run tests
**1.** cmake --open `<lint-combine-build-dir>`

**2.** Choose Solution Configuration **Release**

**3.** In Visual Studio go to: *Extensions→ReSharper→Unit Tests*.

**4.** Choose **Run All Tests from Solution**.

**5.** Tests passed successfully if the number of *Failed Tests* is zero.

### 4. Get lint-combine in Visual Studio, Windows
You can use **lint-combine** via the ReSharper C++ Visual Studio extension. 

#### 4.1 Add path to cpp-lint-combine. 
Add `<lint-combine-build-dir>/Release` to the environment variable `PATH`. (e.g. within the cpp-lint-combine.cmd bootstrapper script).

#### 4.2 Install ReSharper 
Lint-combine works in **ReSharper C++**, so you must install it. 
You can install **ReSharper C++** from [here](https://www.jetbrains.com/resharper-cpp/) (free trial).

#### 4.3 Install supported linters
Lint-combine supports the following linters:
- **[clazy](https://github.com/KDE/clazy)** - get pre-build binaries from [here](https://downloads.kdab.com/clazy/)
- **[clang-tidy](https://clang.llvm.org/extra/clang-tidy/)** — installed with ReSharper C++

Add path to the linters to the environment variable `PATH` (e.g. within the cpp-lint-combine.cmd bootstrapper script).

#### 4.4 Set up ReSharper to use
**1.** In Visual Studio go to: *Extentions→ReSharper→Options→Code Editing→C++→Clang-Tidy*.

**2.** Choose **Custom** in *Clang-Tidy executable to use*.

**3.** Set path to `lint-combine-source-dir/cpp-lint-combine.cmd`.

**4.** Tweak other ReSharper C++ clang-tidy [settings](https://www.jetbrains.com/help/resharper/Clang_Tidy_Integration.html) as desired.
 
#### 4.5 Set your own checks for clazy
**1.** Open the `lint-combine-source-dir/cpp-lint-combine.cmd` file in a text editor.
 
**2.** Specify desired checks/levels in the ```CLAZY_CHECKS``` variable.
