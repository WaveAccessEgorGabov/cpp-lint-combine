# Set the name of the used IDE
IDE_PROFILE="<chooce one from list: CLION, RESHARPER>"

# Set paths to directories with IDE's clang-tidy. If you doesn't have some of this IDE - ignore it
CLION_CLANG_TIDY_PATH="<Path to the directory with CLion's clang-tidy>"
RESHARPER_DIR="$LocalAppData/JetBrains/Installations/ReSharperPlatformVs15_689a5022"
RESHARPER_CLANG_TIDY_PATH="$RESHARPER_DIR/x86:$RESHARPER_DIR"_000/x86""

# Set path to clazy
CLAZY_PATH="<Path to the directory with clazy>"

# Set path to cpp-lint-combine
CPP_LINT_COMBINE_PATH="<Path to the directory with cpp-lint-combine executable>"

# Set clazy checks
CLAZY_CHECKS_MANUAL="assert-with-side-effects,container-inside-loop,detaching-member,heap-allocated-small-trivial-type,ifndef-define-typo,inefficient-qlist,isempty-vs-count,jni-signatures,qhash-with-char-pointer-key,qproperty-type-mismatch,qrequiredresult-candidates,qstring-varargs,qt-keywords,qt4-qstring-from-array,qt6-qlatin1string-to-u,qvariant-template-instantiation,raw-environment-function,reserve-candidates,signal-with-return-value,thread-with-slots,tr-non-literal,unneeded-cast"
CLAZY_CHECKS="level0,level1,level2,$CLAZY_CHECKS_MANUAL"

#Set clang extra args
CLANG_EXTRA_ARGS="-w"

if [ $IDE_PROFILE = "RESHARPER" ]; then
    IDE_CLANG_TIDY_PATH=$RESHARPER_CLANG_TIDY_PATH
fi

if [ $IDE_PROFILE = "CLION" ]; then
    IDE_CLANG_TIDY_PATH=$CLION_CLANG_TIDY_PATH
fi
export PATH="$CLAZY_PATH:$IDE_CLANG_TIDY_PATH:$CPP_LINT_COMBINE_PATH:$PATH"

cpp-lint-combine "--ide-profile=$IDE_PROFILE" "--clazy-checks=$CLAZY_CHECKS" "--clang-extra-args=$CLANG_EXTRA_ARGS" "$@"
