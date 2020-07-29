# ***Cpp-lint-combine***

## 1. Overview

> [**lint**, or a **linter**,](https://en.wikipedia.org/wiki/Lint_(software)) is a tool that analyzes source code to flag programming errors, bugs, stylistic errors, and suspicious constructs.

***Cpp-lint-combine*** is a tool that lets you combine several linters and use them all in an IDE that supports only some (e.g. only one).
### 1.1. Using ***Cpp-lint-combine***

You can use ***Cpp-lint-combine*** as a **command line tool** or in IDEs/ IDE extensions.

### 1.2. Supported linters

 - ***[Clazy](https://github.com/KDE/clazy)***

 - ***[clang-tidy](https://clang.llvm.org/extra/clang-tidy/)***

### 1.3. Supported IDEs/ IDE extensions

 - [***ReSharper C++***](https://www.jetbrains.com/resharper-cpp/) — Visual Studio extension

 - [***CLion***](https://www.jetbrains.com/clion/) — a cross-platform IDE for C and C++

## 2. Get the required tools

### 2.1. ***Git***

Any recent version will do. Install from [here](https://git-scm.com/download).

On *Windows* ensure that ***Git***'s `bin` directory is listed in `PATH` __earlier__ than any other directory containing `sh.exe` (or any other file invoked by the `sh` command).

### 2.2. ***CMake***
Minimum required version is **3.14**. Install from [here](https://cmake.org/download/).

### 2.3. ***Boost***

Minimum required version is **1.69.0**, but you cannot use version **1.72.0** because it contains [an error in Boost.Process](https://github.com/boostorg/process/issues/116).
You can get ***Boost*** on your computer in one of the following ways:

 **1.** Download and build (at least *Datetime*, *Regex*, *ProgramOptions*, and *FileSystem*) from [sources](https://www.boost.org/users/download/).

 **2.** On *Windows*: download and install [prebuilt *Windows* binaries](https://sourceforge.net/projects/boost/files/boost-binaries/) into “`<boost-dir>`” (substitute a path of your choosing).

 **3.** On *Linux*: install ***Boost*** by your package manager (e.g. `apt-get install boost-devel` or `yum install boost-devel`).

## 3. Build ***Cpp-lint-combine***

### 3.1. *Windows*

```sh
git clone https://github.com/WaveAccessEgorGabov/cpp-lint-combine.git <cpp-lint-combine-source-dir>
pushd <cpp-lint-combine-source-dir>
git checkout develop
git submodule update --init --recursive yaml-cpp
popd
cmake -S <cpp-lint-combine-source-dir> -B <cpp-lint-combine-build-dir> -DBOOST_ROOT=<boost-dir>
cmake --build <cpp-lint-combine-build-dir> --config Release
```

### 3.2. *Linux*

```sh
git clone https://github.com/WaveAccessEgorGabov/cpp-lint-combine.git <cpp-lint-combine-source-dir>
pushd <cpp-lint-combine-source-dir>
git checkout develop
git submodule update --init --recursive yaml-cpp
popd
cmake -S <cpp-lint-combine-source-dir> -B <cpp-lint-combine-build-dir> -DCMAKE_BUILD_TYPE=Release
cmake --build <cpp-lint-combine-build-dir>
```

## 4. Run tests

You can ensure that ***Cpp-lint-combine*** built correctly in one of the following ways:

### 4.1. Via Windows CLI

 **1.** Run `<cpp-lint-combine-build-dir>\test\Release\cpp-lint-combine_tests.exe`.

 **2.** Tests passed successfully if the message `*** No errors detected` (green) was output.

### 4.2. Via Linux CLI

 **1.** Run `<cpp-lint-combine-build-dir>/test/cpp-lint-combine_tests`.

 **2.** Tests passed successfully if the message `*** No errors detected` (green) was output.

### 4.3. Via ***ReSharper***

 **1.** Run `cmake --open <cpp-lint-combine-build-dir>` — ***Visual Studio*** opens the ***CMake***-generated solution.

 **2.** Choose Solution Configuration: **Release** (e.g. in the toolbar).

 **3.** Choose *Extensions→ReSharper→Unit Tests→Run All Tests from Solution*.

 **4.** Tests passed successfully if the number of *Failed Tests* is zero.

### 4.4. Via ***CLion***

 **1.** Open the `<cpp-lint-combine-source-dir>` directory by ***CLion***.

 **2.** In *Run/Debug Configuration* choose the **cpp-lint-combine_tests** configuration.

 **3.** Invoke the Run action (e.g. via the green Play-like button).

 **4.** Tests passed successfully if the message `*** No errors detected` (surprisingly, dark red) was output.

## 5. Get ***Cpp-lint-combine*** in Visual Studio (Windows)

### 5.1. Install ***Visual Studio*** and ***ReSharper C++***

***Cpp-lint-combine*** works via ***Visual Studio*** extension ***ReSharper C++***, so you need to install both ***Visual Studio*** and ***ReSharper C++*** in this case.

Required ***Visual Studio*** version is **2017+**. You can install ***Visual Studio 2019*** from [here](https://visualstudio.microsoft.com/downloads/).

You can install ***ReSharper C++*** from [here](https://www.jetbrains.com/resharper-cpp/) (free trial).

### 5.2. Install supported linters

 - [***Clazy***](https://github.com/KDE/clazy) — get pre-built binaries (v1.7+) from [here](https://downloads.kdab.com/clazy/).

 - [***clang-tidy***](https://clang.llvm.org/extra/clang-tidy/) — installed with ReSharper C++.

### 5.3. Set up ***ReSharper*** to use ***Cpp-lint-combine***

 **1.** In ***Visual Studio*** choose *Extensions→ReSharper→Options→Code Editing→C++→Clang-Tidy*.

 **2.** Choose **Custom** in *Clang-tidy executable to use*.

 **3.** Set path to `<cpp-lint-combine-source-dir>/cpp-lint-combine.cmd` (subject to customizations described below).

 **4.** Tweak other ***ReSharper C++*** [***clang-tidy*** settings](https://www.jetbrains.com/help/resharper/Clang_Tidy_Integration.html) as desired.

## 6. Get ***Cpp-lint-combine*** in *CLion*

### 6.1. Install *CLion*

***Cpp-lint-combine*** works in ***CLion***, so you need to install it if you want to use ***Cpp-lint-combine*** via ***CLion***.
You can install ***CLion*** from [here](https://www.jetbrains.com/clion/download/#section=windows) (free trial).

### 6.2. Install supported linters

 - [***Clazy***](https://github.com/KDE/clazy) — get pre-built binaries (v1.7+) from [here](https://downloads.kdab.com/clazy/).

 - [***clang-tidy***](https://clang.llvm.org/extra/clang-tidy/) — installed with ***CLion***.

### 6.3. Set up *CLion* to use ***Cpp-lint-combine***

 **1.** In ***CLion*** choose *Settings→Languages & Frameworks→C/C++→Clang-Tidy*.

 **2.** Tick **Use external Clang-Tidy instead of the built-in one**.

 **3.** Set path to `<cpp-lint-combine-source-dir>/cpp-lint-combine` `.cmd`/`.sh` script on Windows/Linux respectively (subject to customizations described below).

 **4.** Tweak other ***CLion*** [***clang-tidy*** settings](https://www.jetbrains.com/help/clion/clang-tidy-checks-support.html#generalsettings) as desired.

## 7. Configure the `cpp-lint-combine.sh` bootstrapper script

You need to configure `cpp-lint-combine.sh` before using ***Cpp-lint-combine***.

**Note:** in *Windows*, use ***Cygwin***-style paths — with forward slashes, with a leading (root) slash, and without the colon after drive letters, e.g. `/C/Program Files/Git`. 

### 7.1. Choose IDE to run ***Cpp-lint-combine***

Assign a value to the script's variable `IDE_PROFILE`: choose either `ReSharper` or `CLion`.

### 7.2. Set paths

 **1.** Set path to the directory with the `cpp-lint-combine` executable to the script's variable `CPP_LINT_COMBINE_PATH`.

 **2.** Set path to the directory with ***Clazy*** binaries to the script's variable `CLAZY_PATH`.

 **3.** Set paths to directories with IDEs' ***clang-tidy*** executables to the script's variables `<IDE_NAME>_CLANG_TIDY_PATH`. If you don't have some IDE, just ignore it.

### 7.3. Configure ***Clazy***'s checks/levels

You can configure ***Clazy***'s checks/levels in the script's variable `CLAZY_CHECKS`. By default all ***Clazy***'s checks are included.

### 7.4. Configure extra clang arguments

You can add extra clang arguments to the script's variable `CLANG_EXTRA_ARGS` — they will be used by ***Clazy***.  
The default is `"-w"`, see [clang docs](https://clang.llvm.org/docs/ClangCommandLineReference.html) for (much) more details.

### 7.5. Choose linters to use

Set `--sub-linter` `cpp-lint-combine`'s command line argument value. You can use this option several times to use multiple linters. All linters are used by default (if the option isn't set).

## 8. Q&A/ troubleshooting

### 8.1. How to set up ***Cpp-lint-combine*** for both ReSharper and CLion on the same Windows machine?

 **1.** Copy `cpp-lint-combine.sh` and `cpp-lint-combine.cmd` under the same new base name (e.g. `cpp-lint-combine-CLion.cmd` and `cpp-lint-combine-CLion.sh`).

 **2.** Configure distinct bootstrapper script for each IDE — please refer to the “*Configure the `cpp-lint-combine.sh` bootstrapper script*” section above.

### 8.2. Issue: not seeing ANY inspection messages from ***clang-tidy*** among ReSharper's ones

 Check ***Cpp-lint-combine*** diagnostics:

  **1.** Run ***Visual Studio*** (`devenv.exe`) with option `/ReSharper.Internal`.

  **2.** Open the solution and the source file.

  **3.** In Visual Studio choose: *Extensions→ReSharper→Internal→C++→Dump clang-tidy output*.

  **4.** Wait a few seconds (till a text files opens externally) and check warning/errors in the file's “Clang-tidy stderr:” section.
