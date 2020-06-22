@echo off
PATH lint-combine-build-dir\Release;%LocalAppData%\JetBrains\Installations\ReSharperPlatformVs15_689a5022_000\x86;Path\to\clazy
set CLAZY_CHECKS_MANUAL=assert-with-side-effects,container-inside-loop,detaching-member,heap-allocated-small-trivial-type,ifndef-define-typo,inefficient-qlist,isempty-vs-count,jni-signatures,qhash-with-char-pointer-key,qproperty-type-mismatch,qrequiredresult-candidates,qstring-varargs,qt-keywords,qt4-qstring-from-array,qvariant-template-instantiation,raw-environment-function,reserve-candidates,signal-with-return-value,thread-with-slots,tr-non-literal,unneeded-cast
set "CLAZY_CHECKS=level0,level1,%CLAZY_CHECKS_MANUAL%"
set "CLANG_EXTRA_ARGS=-Wno-c++98-compat -Wno-missing-prototypes"
cpp-lint-combine.exe "--clazy-checks=%CLAZY_CHECKS%" "--clang-extra-args=%CLANG_EXTRA_ARGS%" %*
