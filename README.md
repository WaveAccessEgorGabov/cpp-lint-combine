# Lint-combine

### 1. Overview
Lint-combine is a tool that lets you combine several [linters](https://en.wikipedia.org/wiki/Lint_(software)).
#### 1.1 What is a linter
**lint**, or a **linter**, is a tool that analyzes source code to flag programming errors, bugs, stylistic errors, and suspicious constructs. 
#### 1.2 Using lint-combine
You can use lint-combine as a **command line tool**, or in [**ReSharper C++**](https://www.jetbrains.com/resharper-cpp/) — Visual Studio extension. 

### 2. Get the required tools 
- **Boost** — minimum required version is **1.73.0**. Install from [here](https://www.boost.org/users/history/version_1_73_0.html).
- **CMake** — minimum required version is **3.14**. Install from [here](https://cmake.org/download/).
- **Git** — Install from [here](https://git-scm.com/download).
- **Visual Studio** — minimum required version is **201***. You can install Visual Studio 2019 from [here](https://visualstudio.microsoft.com/ru/downloads/)

### 3. Build lint-combine  
#### Windows
```sh
git clone https://github.com/WaveAccessEgorGabov/cpp-lint-combine.git <lint-combine-source-dir>
git checkout develop
cmake -S <lint-combine-source-dir> -B <lint-combine-build-dir>
cmake --build <lint-combine-build-dir> --config Release
```

Add path to `lint-combine-build-dir/bin` to environment variable PATH.

### 4. Get lint-combine in Visual Studio, Windows
You can use **lint-combine** via the ReSharper C++ Visual Studio extension. 
#### 4.1 Install ReSharper 
Lint-combine works in **ReSharper C++**, so you must install it. 
You can install **ReSharper C++** from [here](https://www.jetbrains.com/resharper-cpp/). (free trial)
#### 4.2 Supported linters
Lint-combine supports the following linters:
- **[clazy](https://github.com/KDE/clazy)**
- **[clang-tidy](https://clang.llvm.org/extra/clang-tidy/)** — minimum required version is **10**

It is necessary to install all supported linters.

#### 4.3 Install clang-tidy (installed with ReSharper C++)

#### 4.4 Install clazy
Install llvm-10.
```sh
git clone https://github.com/llvm/llvm-project.git <llvm-source-dir>
cd <llvm-source-dir>
git checkout release/10.x
cmake -S <llvm-source-dir>/llvm -B <lvm-build-dir> -DLLVM_EXPORT_SYMBOLS_FOR_PLUGINS=ON -A x64 -Thost=x64 -DLLVM_ENABLE_PROJECTS=clang 
cmake --build <llvm-build-dir> --config Release
```

Add path to `lvm-build-dir/bin` to environment variable PATH.

```sh
git clone https://github.com/KDE/clazy.git <clazy-source-dir>
cd <clazy-source-dir>
git checkout 1.6
cmake -S <clazy-source-dir> -B <clazy-build-dir> -DCLANG_LIBRARY_IMPORT=<llvm-build-dir>/lib/clang.lib -DCMAKE_BUILD_TYPE=Release
cmake --build <clazy-build-dir> --config Release
```

Add path to `clazy-build-dir/bin` to environment variable PATH.

#### 4.5 Set up ReSharper to use
**1.** In Visual Studio go to: *Extentensions—>ReSharper—>Options—>Code Editing—>C++—>Clang-Tidy*.

**2.** Choose **Custom** in *Clang-Tidy executable to use*.

**3.** Set path to `lint-combine-source-dir/cpp-lint-combine.cmd`.

#### 4.6 Set your own check for clang-tidy
**1.** In Visual Studio go to: *Extentensions—>ReSharper—>Options—>Code Editing—>C++—>Clang-Tidy*.

**2.** Write to *List of enabled/disabled clang-tidy checks* your own checks.

#### 4.7 Set your own checks for clazy
**1.** Open file **lint-combine-source-dir/cpp-lint-combine.cmd**.
 
**2.** Specify desired checks/levels in the CLAZY_CHECKS variable.
