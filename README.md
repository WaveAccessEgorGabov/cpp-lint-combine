# Lint-combine

### 1. Overview
Lint-combine is a tool that lets you combine several [linters](https://en.wikipedia.org/wiki/Lint_(software)) and use them all in an IDE that supports only some (e.g. only one).
#### 1.1 What is a linter
**lint**, or a **linter**, is a tool that analyzes source code to flag programming errors, bugs, stylistic errors, and suspicious constructs. 
#### 1.2 Using lint-combine
You can use lint-combine as a **command line tool**, or in IDEs/IDE extensions.
#### 1.3 List of supported linters
**1.** ***[clazy](https://github.com/KDE/clazy)***

**2.** ***[clang-tidy](https://clang.llvm.org/extra/clang-tidy/)***
#### 1.4 List of supported IDEs/IDE extensions
**1.** [***ReSharper C++***](https://www.jetbrains.com/resharper-cpp/) — Visual Studio extension. 

**2.** [***CLion***](https://www.jetbrains.com/clion/) — A cross-platform IDE for C and C++.

### 2. Get the required tools 
- ***Boost*** — minimum required version is **1.69.0**, but you cannot use version **1.72.0**, because it contains [an error in the boost::process](https://github.com/boostorg/process/issues/116).
You can get Boost on your computer in the following ways. 

**1.** Download and build (at least Datetime, Regex, ProgramOptions, and FileSystem) from [sources](https://www.boost.org/users/download/).

**2.** On Windows: download and install [prebuilt Windows binaries](https://sourceforge.net/projects/boost/files/boost-binaries/) into “`<boost-dir>`” (substitute a path of your choosing).

**3.** On Linux: install from your package manager into “`<boost-dir>`” (substitute a path of your choosing).

- ***CMake*** — minimum required version is **3.14**. Install from [here](https://cmake.org/download/).
- ***Git*** — install from [here](https://git-scm.com/download). On Windows ensure that Git's `bin` directory is listed in `PATH` earlier than any other directory containing `sh.exe` (or any other file invoked by the `sh` command).

### 3. Build lint-combine  
#### Windows
```sh
git clone https://github.com/WaveAccessEgorGabov/cpp-lint-combine.git <lint-combine-source-dir>
cd <lint-combine-source-dir>
git checkout develop
git submodule update --init --recursive yaml-cpp
cmake -S <lint-combine-source-dir> -B <lint-combine-build-dir> -DBOOST_ROOT=<boost-dir>
cmake --build <lint-combine-build-dir> --config Release
```
#### Linux
```sh
git clone https://github.com/WaveAccessEgorGabov/cpp-lint-combine.git <lint-combine-source-dir>
cd <lint-combine-source-dir>
git checkout develop
git submodule update --init --recursive yaml-cpp
cmake -S <lint-combine-source-dir> -B <lint-combine-build-dir> -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=<boost-dir>
cmake --build <lint-combine-build-dir>
```

### 4. Run tests 
#### Through *ReSharper*
**1.** `cmake --open <lint-combine-build-dir>`

**2.** Choose Solution Configuration **Release**

**3.** In Visual Studio choose: *Extensions→ReSharper→Unit Tests→Run All Tests from Solution*.

**4.** Tests passed successfully if the number of *Failed Tests* is zero.

#### Through *CLion*
**1.** Open cpp-lint-combine directory through ***CLion***.

**2.** In *Run/Debug Configuration* choose cpp-lint-combineTesting configuration.

**3.** Run cpp-lint-combineTesting

**4.** Tests passed successfully if the message `*** No errors detected` displayed.

#### Through CLI

##### Windows
**1.** Run `<lint-combine-build-dir>/test/Release/cpp-lint-combineTesting`

**2.** Tests passed successfully if the message `*** No errors detected` displayed.

##### Linux
**1.** Run `<lint-combine-build-dir>/test/cpp-lint-combineTesting`

**2.** Tests passed successfully if the message `*** No errors detected` displayed.

### 5. Get lint-combine in Visual Studio, Windows 

#### 5.1 Install *Visual Studio* and *ReSharper C++* 
Lint-combine works in ***Visual Studio*** extension ***ReSharper C++***, so you must install ***Visual Studio*** and ***ReSharper C++*** if you want to use ***cpp-lint-combine*** through ***ReSharper C++***.

You can install ***Visual Studio 2019*** from [here](https://visualstudio.microsoft.com/downloads/). ***Visual Studio*** — required version is **2017+**. 

You can install ***ReSharper C++*** from [here](https://www.jetbrains.com/resharper-cpp/) (free trial).

#### 5.2 Install supported linters
Lint-combine supports the following linters:
- ***[clazy](https://github.com/KDE/clazy)*** — get pre-built binaries from [here](https://downloads.kdab.com/clazy/)
- ***[clang-tidy](https://clang.llvm.org/extra/clang-tidy/)*** — installed with ReSharper C++

#### 5.3 Set up *ReSharper* to use
**1.** In Visual Studio choose: *Extentions→ReSharper→Options→Code Editing→C++→Clang-Tidy*.

**2.** Choose **Custom** in *Clang-Tidy executable to use*.

**3.** Set path to `lint-combine-source-dir/cpp-lint-combine.cmd`.

**4.** Tweak other ReSharper C++ clang-tidy [settings](https://www.jetbrains.com/help/resharper/Clang_Tidy_Integration.html) as desired.

### 6. Get lint-combine in *CLion* 

#### 6.1 Install *CLion* 
Lint-combine works in ***CLion***, so you must install it if you want to use ***cpp-lint-combine*** through ***CLion***. 
You can install ***CLion*** from [here](https://www.jetbrains.com/clion/download/#section=windows) (free trial).

#### 6.2 Install supported linters
Lint-combine supports the following linters:
- ***[clazy](https://github.com/KDE/clazy)*** — get pre-built binaries from [here](https://downloads.kdab.com/clazy/)
- ***[clang-tidy](https://clang.llvm.org/extra/clang-tidy/)*** — installed with ***CLion***

Add path to the linters to the environment variable `PATH` (e.g. within the cpp-lint-combine.sh bootstrapper script).

#### 6.3 Set up *CLion* to use
**1.** In ***CLion*** choose: *Settings→Languages & Frameworks→C/C++→Clang-Tidy*.

**2.** Tick **Use external Clang-Tidy instead of the built-in one**.

**3.1** Set path to `lint-combine-source-dir/cpp-lint-combine.cmd` on Windows.

**3.2** Set path to `lint-combine-source-dir/cpp-lint-combine.sh` on Linux.

### 7. Configure cpp-lint-combine.sh bootstrapper script
You must configure cpp-lint-combine.sh before using cpp-lint-combine.

#### 7.1 Choose IDE in which cpp-lint-combine will run
Set value to the script's variable `IDE_PROFILE`. Chooce one from list: CLion, ReSharper.

#### 7.2 Set paths
**1.** Set path to `<lint-combine-build-dir>/Release` to the script's variable `CPP_LINT_COMBINE_PATH`.

**2.** Set paths to directories with IDE's clang-tidy to to the scripts variables `<IDE_NAME>_CLANG_TIDY_PATH`. If you don't have some IDE, just ignore it.

**3.** Set path to the directory with clazy to the script's variable `CLAZY_PATH`.

#### 7.3 Configure clazy's checks
You can configure clazy's checks in the script's variable **CLAZY_CHECKS**. By default all clazy's checks are included.

#### 7.4 Configure clang extra arguments checks
You can add clang extra arguments to the script's variable **CLANG_EXTRA_ARGS**. Clang extra arguments will be used by ***clazy***. The defaults is **"-w"**.

#### 7.5 Choose linters to use
Configure **sub-linter** value (***cpp-lint-combine***'s command line argument). You can use this param several times to set several linters. All linters will used by default if the option isn't set. 

### 8. Troubleshooting
#### How do I set up cpp-lint-combine for both ReSharper and CLion on the same Windows machine?
**Solution**: 
**1.** Copy cpp-lint-combine.sh and cpp-lint-combine.cmd and save them under the same name (e.g. copied files names could be cpp-lint-combine-copy.cmd and cpp-lint-combine-copy.sh).

**2.** Configure different bootstrapper scripts for different IDE. How to configure bootstrapper scripts you can find in the section *Configure cpp-lint-combine.sh bootstrapper script*.

#### Issue: not seeing any inspection messages
**Solution**: check cpp-lint-combine diagnostics. 
For this:
**1.** Run Visual studio (devenv.exe) with option `/ReSharper.Internal`.

**2.** In Visual Studio choose: *Extentions→ReSharper→Internal→C++→Dump clang-tidy output*.

**3.** Wait a few seconds and check warning/errors in the file that opens in the clang-tidy stdout/stderr.
