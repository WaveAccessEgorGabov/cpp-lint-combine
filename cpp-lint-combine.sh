#!/bin/bash
# Set the name of the used IDE
IDE_PROFILE="<choose one of these: CLion, ReSharper>"

# Set path to cpp-lint-combine. In Windows, write it as /D/no-colon/forward-slashes.
CPP_LINT_COMBINE_PATH="<Full directory path of the cpp-lint-combine executable>"

# Set path to clazy. In Windows, write it as /D/no-colon/forward-slashes.
CLAZY_PATH="<Full directory path of the clazy executable>"

# Set path to the IDE's clang-tidy. If you don't have some IDE, just ignore it.
CLION_CLANG_TIDY_PATH="<Full directory path of the CLion's clang-tidy>"

# Path to ReSharper's clang-tidy is already set. You shouldn't need to change it.
PATH_TO_JETBRAINS_INSTALLATIONS="/${LOCALAPPDATA/:}/JetBrains/Installations"
RESHARPER_DIR="$PATH_TO_JETBRAINS_INSTALLATIONS/$(ls $PATH_TO_JETBRAINS_INSTALLATIONS | grep ReSharperPlatformVs | tail -1)"
RESHARPER_CLANG_TIDY_PATH="${RESHARPER_DIR}/x86:${RESHARPER_DIR}_000/x86"

# Set clazy checks
CLAZY_CHECKS_MANUAL="assert-with-side-effects,container-inside-loop,detaching-member,heap-allocated-small-trivial-type,ifndef-define-typo,inefficient-qlist,isempty-vs-count,jni-signatures,qhash-with-char-pointer-key,qproperty-type-mismatch,qrequiredresult-candidates,qstring-varargs,qt-keywords,qt4-qstring-from-array,qt6-qlatin1string-to-u,qvariant-template-instantiation,raw-environment-function,reserve-candidates,signal-with-return-value,thread-with-slots,tr-non-literal,unneeded-cast"
CLAZY_CHECKS="level0,level1,level2,$CLAZY_CHECKS_MANUAL"

# Set clang extra args
CLANG_EXTRA_ARGS="-w"

if [ "${IDE_PROFILE,,}" = "resharper" ]; then
    IDE_CLANG_TIDY_PATH=$RESHARPER_CLANG_TIDY_PATH
fi

if [ "${IDE_PROFILE,,}" = "clion" ]; then
    IDE_CLANG_TIDY_PATH=$CLION_CLANG_TIDY_PATH
fi

if [ "${IDE_PROFILE,,}" = "baremsvc" ]; then
    IDE_CLANG_TIDY_PATH="$(dirname "$0")"
	IDE_CLANG_TIDY_PATH="/${IDE_CLANG_TIDY_PATH/:}"
fi

export PATH="$CLAZY_PATH:$IDE_CLANG_TIDY_PATH:$PATH"

"$CPP_LINT_COMBINE_PATH/cpp-lint-combine" "--ide-profile=$IDE_PROFILE" "--clazy-checks=$CLAZY_CHECKS" "--clang-extra-args=$CLANG_EXTRA_ARGS" "$@"
