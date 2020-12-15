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
    printf '%*s' $1 "a"
}

# $1 - Linter output offset
# $2 - Linter output
# $3 - Converted linter output
# $4 - Print linter output or expected result
# $5 - Number of new lines (\n) in the linter output
generateStringForDifferentBufferSize() {
    for i in {512..512}
    do
        printf -v spaceFromBuffersBeginToMessage '%*s' $(($i-$1)) "a"
        printf -v spaceFromMessageToBuffersEnd '%*s' $(($i-$(($(($i-${1}+${#2}-${5})) % $i)) - 1)) "a"

        if $4; then
            printf "${spaceFromBuffersBeginToMessage}${2}${spaceFromMessageToBuffersEnd}\n"
        else
            printf "${spaceFromBuffersBeginToMessage}${3}${spaceFromMessageToBuffersEnd}\n"
        fi
    done
}

commonCheckPart=" warning: check_text "
fileName="mai(42,42)n.cpp"
pathToFile="C:\some\pa(42,42)th\\"${fileName}

rowColumnSource="(42,42):"
rowColumnConverted=":42:42:"
checkNameSource="[-wsome-check]"
checkNameConverted="[wsome-check]"
testStrSource="${pathToFile}${rowColumnSource}${commonCheckPart}${checkNameSource}\n"
testStrConverted="${pathToFile}${rowColumnConverted}${commonCheckPart}${checkNameConverted}\n"
newLineSymSize=2

# Test 1 - Check's message on the border of buffer
generateStringForDifferentBufferSize $((${#testStrSource} - ${newLineSymSize})) \
                                     "$testStrSource" "$testStrConverted" $getTests 1

# Test 2 - Check's message split between buffers in the following way:
# <some_text>[wsome-check
# ]<some_text>
generateStringForDifferentBufferSize $((${#testStrSource} - ${newLineSymSize} - 1)) \
                                     "$testStrSource" "$testStrConverted" $getTests 1

# Test 3 - Check's message split between buffers in the following way:
# <some_text>[wsome
# -check]<some_text>
generateStringForDifferentBufferSize $((${#testStrSource} - ${newLineSymSize} - ${#checkNameSource} / 2)) \
                                     "$testStrSource" "$testStrConverted" $getTests 1
# Test 4 - Check's message split between buffers in the following way:
# <some_text>[
# wsome-check]<some_text>
generateStringForDifferentBufferSize $((${#testStrSource} - ${newLineSymSize} - ${#checkNameSource} + 1)) \
                                     "$testStrSource" "$testStrConverted" $getTests 1

# Test 5 - Check's message split between buffers in the following way:
# <some_text>
# [wsome-check]<some_text>
generateStringForDifferentBufferSize $((${#testStrSource} - ${newLineSymSize} - ${#checkNameSource})) \
                                     "$testStrSource" "$testStrConverted" $getTests 1

# Test 6 - Check's message split between buffers in the following way:
# <some_text>mai(42,42)n.cpp(42,42)
# <some_text>: warning
generateStringForDifferentBufferSize $((${#testStrSource} - ${newLineSymSize} - \
                                     ${#checkNameSource} - ${#commonCheckPart} - 1)) \
                                     "$testStrSource" "$testStrConverted" $getTests 1

# Test 7 - Check's message split between buffers in the following way:
# <some_text>mai(42,42)n.cpp(42
# <some_text>,42): warning
generateStringForDifferentBufferSize $((${#testStrSource} - ${newLineSymSize} - ${#checkNameSource} - \
                                     ${#commonCheckPart} - ${#rowColumnSource} / 2 - 1)) \
                                     "$testStrSource" "$testStrConverted" $getTests 1

# Test 8 - Check's message split between buffers in the following way:
# <some_text>mai(42,42)n.cpp(
# <some_text>42,42): warning
generateStringForDifferentBufferSize $((${#testStrSource} - ${newLineSymSize} - ${#checkNameSource} - \
                                     ${#commonCheckPart} - ${#rowColumnSource} + 1)) \
                                     "$testStrSource" "$testStrConverted" $getTests 1

# Test 9 - Check's message split between buffers in the following way:
# <some_text>mai(42,42)n.cpp
# <some_text>(42,42): warning
generateStringForDifferentBufferSize $((${#testStrSource} - ${newLineSymSize} - ${#checkNameSource} - \
                                     ${#commonCheckPart} - ${#rowColumnSource})) \
                                     "$testStrSource" "$testStrConverted" $getTests 1

# Test 10 - Check's message split between buffers in the following way:
# <some_text>warning :
# <some_text>check_text
generateStringForDifferentBufferSize $((${#testStrSource} - ${newLineSymSize} - \
                                     ${#checkNameSource} - ${#commonCheckPart} / 2 )) \
                                     "$testStrSource" "$testStrConverted" $getTests 1

# Test 11 - Path to file is short (e.g. "C:\"). Check's message split between buffers in the following way:
# path_to_file\
# <filename>(42,42):
shortPathToFile="C:\\"${fileName}
testStrSource="${shortPathToFile}${rowColumnSource}${commonCheckPart}${checkNameSource}\n"
testStrConverted="${shortPathToFile}${rowColumnConverted}${commonCheckPart}${checkNameConverted}\n"
generateStringForDifferentBufferSize $((${#testStrSource} - ${newLineSymSize} - \
                                     ${#checkNameSource} - ${#commonCheckPart} - \
                                     ${#rowColumnSource} - ${#fileName})) \
                                     "$testStrSource" "$testStrConverted" $getTests 1

# Test 12 - Check's message completely in buffer
generateStringForDifferentBufferSize $((${#testStrSource} + 10)) \
                                     "$testStrSource" "$testStrConverted" $getTests 1

# Test 13 - Two check's message completely in buffer
generateStringForDifferentBufferSize $((${#testStrSource} * 2 + 20)) \
                                     "$testStrSource$testStrSource" \
                                     "$testStrConverted$testStrConverted" $getTests 2

# Test 14 - Two check's message completely in buffer, some text between messages exist
generateStringForDifferentBufferSize $(((${#testStrSource}) * 2 + 30)) \
                                     "${testStrSource}"HelloWorld"${testStrSource}" \
                                     "${testStrConverted}"HelloWorld"${testStrConverted}" $getTests 2

# Test 15 - Test that all hyphen removed from check's name
checkNameSource="[----wsome-check]"
checkNameConverted="[wsome-check]"
testStrSource="${pathToFile}${rowColumnSource}${commonCheckPart}${checkNameSource}\n"
testStrConverted="${pathToFile}${rowColumnConverted}${commonCheckPart}${checkNameConverted}\n"
generateStringForDifferentBufferSize $((${#testStrSource} + 10)) \
                                     "$testStrSource" "$testStrConverted" $getTests 1

# Test 16 - The message starts with file path that isn't related to check message
mesBegin="${pathToFile}:"
testStrSource="${mesBegin}${pathToFile}${rowColumnSource}${commonCheckPart}${checkNameSource}\n"
testStrConverted="${mesBegin}${pathToFile}${rowColumnConverted}${commonCheckPart}${checkNameConverted}\n"
generateStringForDifferentBufferSize $((${#testStrSource} + 10)) \
                                     "$testStrSource" "$testStrConverted" $getTests 1

# Test 17 - The message ends with file path that isn't related to check message
mesEnd="${pathToFile}:"
testStrSource="${pathToFile}${rowColumnSource}${commonCheckPart}${checkNameSource}${mesEnd}\n"
testStrConverted="${pathToFile}${rowColumnConverted}${commonCheckPart}${checkNameConverted}${mesEnd}\n"
generateStringForDifferentBufferSize $((${#testStrSource} + 10)) \
                                     "$testStrSource" "$testStrConverted" $getTests 1

# Test 18
rowColumnPart="(aa,bb):" # symbol instead of numbers
checkNamePart="[wsome-check---]"
testStr="${pathToFile}${rowColumnPart}${commonCheckPart}${checkNamePart}\n"
generateStringForDifferentBufferSize $((${#testStr} + 10)) \
                                     "${testStr}" "${testStr}" $getTests 1

# Test 19
rowColumnPart="(42,    42):" # extra spaces added
checkNamePart="[wsome----check]"
testStr="${pathToFile}${rowColumnPart}${commonCheckPart}${checkNamePart}\n"
generateStringForDifferentBufferSize $((${#testStr} + 10)) \
                                     "${testStr}" "${testStr}" $getTests 1

# Test 20
rowColumnPart="(42,42)" # colon does not exist
checkNamePart="[wsome-check"
testStr="${pathToFile}${rowColumnPart}${commonCheckPart}${checkNamePart}\n"
generateStringForDifferentBufferSize $((${#testStr} + 10)) \
                                     "${testStr}" "${testStr}" $getTests 1
