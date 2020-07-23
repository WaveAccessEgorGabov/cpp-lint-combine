CLAZY_CHECKS_MANUAL=assert-with-side-effects,container-inside-loop,detaching-member,heap-allocated-small-trivial-type,ifndef-define-typo,inefficient-qlist,isempty-vs-count,jni-signatures,qhash-with-char-pointer-key,qproperty-type-mismatch,qrequiredresult-candidates,qstring-varargs,qt-keywords,qt4-qstring-from-array,qt6-qlatin1string-to-u,qvariant-template-instantiation,raw-environment-function,reserve-candidates,signal-with-return-value,thread-with-slots,tr-non-literal,unneeded-cast
CLAZY_CHECKS="level0,level1,level2,$CLAZY_CHECKS_MANUAL"
CLANG_EXTRA_ARGS="-w"

cpp-lint-combine "--ide-profile=CLion" "--clazy-checks=$CLAZY_CHECKS" "--clang-extra-args=$CLANG_EXTRA_ARGS" $@