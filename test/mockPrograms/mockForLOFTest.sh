#!/bin/bash

# We generate test's data here, because we can't pass long test
# string via the command line argument into the bash-script
# due to the limitation on the length of the command line argument

while getopts "tr" flag
do
    case "${flag}" in
        t) getTests=true;;
        r) getTests=test;;
    esac
done

fillBuffer() {
    printf '%*s' $1 #| tr ' ' 'a' # for debug
}

generateStringForDifferentBufferSize() {
    for i in {256..8192..256} # TODO: Think about buffer size and step
    do
        spaceFromBuffersBeginToMessage=$(($i-$1))
        spaceFromMessageToBuffersEnd=$(($i-$(($(($i-$1+${#2})) % $i))))

        fillBuffer $spaceFromBuffersBeginToMessage
        if $4; then
            printf "$2" # print linterOutput
        else
            printf "$3" # print converted linterOutput
        fi
        fillBuffer $spaceFromMessageToBuffersEnd
    done
}

commonCheckPart=" warning : check_text "
if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    pathToFile="C:\some\pa(42,42)th\mai(42,42)n.cpp"
else
    pathToFile="/c/some/pa(42,42)th/ma(42,42)in.cpp"
fi

rowColumnSource="(42,42):"
rowColumnConverted=":42:42:"
checkNameSource="[-wsome-check]"
checkNameConverted="[wsome-check]"
testStrSource="${pathToFile}${rowColumnSource}${commonCheckPart}${checkNameSource}"
testStrConverted="${pathToFile}${rowColumnConverted}${commonCheckPart}${checkNameConverted}"

# Test 1 - Check's message on the border of buffer
generateStringForDifferentBufferSize $((${#testStrSource})) \
                                     "$testStrSource" "$testStrConverted" $getTests

# Test 2 - Check's message split between buffers in the following way:
# <some_text>[wsome-check
# ]<some_text>
generateStringForDifferentBufferSize $((${#testStrSource} - 1)) \
                                     "$testStrSource" "$testStrConverted" $getTests

# Test 3 - Check's message split between buffers in the following way:
# <some_text>[wsome
# -check]<some_text>
generateStringForDifferentBufferSize $((${#testStrSource} - ${#checkNameSource} / 2)) \
                                     "$testStrSource" "$testStrConverted" $getTests

# Test 4 - Check's message split between buffers in the following way:
# <some_text>[
# wsome-check]<some_text>
generateStringForDifferentBufferSize $((${#testStrSource} - ${#checkNameSource} + 1)) \
                                     "$testStrSource" "$testStrConverted" $getTests

# Test 5 - Check's message split between buffers in the following way:
# <some_text>
# [wsome-check]<some_text>
generateStringForDifferentBufferSize $((${#testStrSource} - ${#checkNameSource})) \
                                     "$testStrSource" "$testStrConverted" $getTests

# Test 6 - Check's message split between buffers in the following way:
# <some_text>mai(42,42)n.cpp(42,42)
# <some_text>: warning
generateStringForDifferentBufferSize $((${#testStrSource} - ${#checkNameSource} - ${#commonCheckPart} - 1)) \
                                     "$testStrSource" "$testStrConverted" $getTests

# Test 7 - Check's message split between buffers in the following way:
# <some_text>mai(42,42)n.cpp(42
# <some_text>,42): warning
generateStringForDifferentBufferSize $((${#testStrSource} - ${#checkNameSource} - \
                                     ${#commonCheckPart} - ${#rowColumnSource} / 2 - 1)) \
                                     "$testStrSource" "$testStrConverted" $getTests

# Test 8 - Check's message split between buffers in the following way:
# <some_text>mai(42,42)n.cpp(
# <some_text>42,42): warning
generateStringForDifferentBufferSize $((${#testStrSource} - ${#checkNameSource} - \
                                     ${#commonCheckPart} - ${#rowColumnSource} + 1)) \
                                     "$testStrSource" "$testStrConverted" $getTests

# Test 9 - Check's message split between buffers in the following way:
# <some_text>mai(42,42)n.cpp
# <some_text>(42,42): warning
generateStringForDifferentBufferSize $((${#testStrSource} - ${#checkNameSource} - \
                                     ${#commonCheckPart} - ${#rowColumnSource})) \
                                     "$testStrSource" "$testStrConverted" $getTests

# Test 10 - Check's message split between buffers in the following way:
# <some_text>warning :
# <some_text>check_text
generateStringForDifferentBufferSize $((${#testStrSource} - ${#checkNameSource} - ${#commonCheckPart} / 2 )) \
                                     "$testStrSource" "$testStrConverted" $getTests

# Test 11 - Check's message completely in buffer
generateStringForDifferentBufferSize $((${#testStrSource} + 10)) \
                                     "$testStrSource" "$testStrConverted" $getTests

# Test 12 - Two check's message completely in buffer
generateStringForDifferentBufferSize $((${#testStrSource} * 2)) \
                                     "$testStrSource$testStrSource" \
                                     "$testStrConverted$testStrConverted" $getTests

# Test 13 - Two check's message completely in buffer, some text between messages exist
generateStringForDifferentBufferSize $((${#testStrSource} * 2)) \
                                     "${testStrSource}"HelloWorld"${testStrSource}" \
                                     "${testStrConverted}"HelloWorld"${testStrConverted}" $getTests

# Test 14 - Test that all hyphen removed from check's name
checkNameSource="[----wsome-check]"
checkNameConverted="[wsome-check]"
testStrSource="${pathToFile}${rowColumnSource}${commonCheckPart}${checkNameSource}"
testStrConverted="${pathToFile}${rowColumnConverted}${commonCheckPart}${checkNameConverted}"
generateStringForDifferentBufferSize $((${#testStrSource} + 10)) \
                                     "$testStrSource" "$testStrConverted" $getTests

# Tests in which the text is very similar to the message that needs to be changed

# Test 15
rowColumnPart="(aa,bb):" # symbol instead of numbers
checkNamePart="[wsome-check---]"
testStr="${pathToFile}${rowColumnPart}${commonCheckPart}${checkNamePart}"
generateStringForDifferentBufferSize $((${#testStr} + 10)) \
                                     "${testStr}" "${testStr}" $getTests

# Test 16
rowColumnPart="(42,    42):" # extra spaces added
checkNamePart="[wsome----check]"
testStr="${pathToFile}${rowColumnPart}${commonCheckPart}${checkNamePart}"
generateStringForDifferentBufferSize $((${#testStr} + 10)) \
                                     "${testStr}" "${testStr}" $getTests

# Test 17
rowColumnPart="(42,42)" # colon does not exist
checkNamePart="[wsome-check"
testStr="${pathToFile}${rowColumnPart}${commonCheckPart}${checkNamePart}"
generateStringForDifferentBufferSize $((${#testStr} + 10)) \
                                     "${testStr}" "${testStr}" $getTests
