# Set the name of the used IDE
set IDE_PROFILE="<chooce one from list: CLION, RESHARPER>"

# Set paths directory with IDEs clang-tidy. If you doesn't have some of this IDE - ignore it

# Windows examle path: C:\Program Files\JetBrains\CLion <version>\bin\clang\win
set CLION_CLANG_TIDY_PATH="<Path to the directory with CLions clang-tidy>"

# Directory with ReSharper can be changed from version to version.
# Windows example path: %LocalAppData%\JetBrains\Installations\ReSharperPlatformVs15_689a5022
set RESHARPER_DIR="<Path ReSharper's dir>"
set RESHARPER_CLANG_TIDY_PATH="%RESHARPER_DIR%\x86;%RESHARPER_DIR%_000\x86"

# Set path to clazy
set CLAZY_PATH="<Path to the directory with clazy-standalone>"

# Set path to cpp-lint-combine
set CPP_LINT_COMBINE_PATH="<Path to the directory with cpp-lint-combine executable>"

set IDE_CLANG_TIDY_PATH="%IDE_PROFILE%%_CLANG_TIDY_PATH%"
PATH=$CLAZY_PATH;$IDE_CLANG_TIDY_PATH;$CPP_LINT_COMBINE_PATH

# Set clazy checks
CLAZY_CHECKS_MANUAL="assert-with-side-effects,container-inside-loop,detaching-member,heap-allocated-small-trivial-type,ifndef-define-typo,inefficient-qlist,isempty-vs-count,jni-signatures,qhash-with-char-pointer-key,qproperty-type-mismatch,qrequiredresult-candidates,qstring-varargs,qt-keywords,qt4-qstring-from-array,qt6-qlatin1string-to-u,qvariant-template-instantiation,raw-environment-function,reserve-candidates,signal-with-return-value,thread-with-slots,tr-non-literal,unneeded-cast"
CLAZY_CHECKS="level0,level1,level2,$CLAZY_CHECKS_MANUAL"

#Set clang extra args
CLANG_EXTRA_ARGS="-w"

# You can add argument "--sub-linter" to the cpp-lint-combine's command line
# with value from list: clang-tidy, clazy (e.g. --sub-linter=clang-tidy)
# By default all supported linters used
cpp-lint-combine "--ide-profile=CLion" "--clazy-checks=$CLAZY_CHECKS" "--clang-extra-args=$CLANG_EXTRA_ARGS" $@ 2> /home/egor/Documents/WaveAccess/WavePoint_General/cpp-lint-combine/DUMP.txt
