#!/usr/bin/env bash
# Run with ./test.sh (you must be in the same directory)

avengers="$PWD/cmake-build-debug/avengers"
code_asm="../../TP2/parm_public/code_asm/test_integration/"
code_c="../../TP2/parm_public/code_c/"

function run_test(){
    pushd "$1" > /dev/null

    for file in $(find . -name "*.s")
    do
        echo "Testing file $file"
        "$avengers" "$file" &> /dev/null || echo "Failed to build file $file!"
        git diff --name-status -- "${file/.s/.bin}"
    done

    popd > /dev/null
}

run_test "$code_asm"
run_test "$code_c"