#!/usr/bin/env bash
# Run with ./test.sh (you must be in the same directory)
# parm_public (https://bitbucket.org/edge-team-leat/parm_public)
# needs to be cloned in the same parent directory as this repo
# avengers must be compiled in the directory cmake-build-debug

avengers="$PWD/cmake-build-debug/avengers"
code_asm="../../parm_public/code_asm/test_integration/"
code_c="../../parm_public/code_c/"

function run_test(){
    pushd "$1" > /dev/null

    for file in $(find . -name "*.s")
    do
        echo "Testing file $file"
        "$avengers" "$file" &> /dev/null || echo "Failed to build file $file!"
        git diff --name-status --exit-code -- "${file/.s/.bin}" && echo "no difference"
    done

    popd > /dev/null
}

run_test "$code_asm"
run_test "$code_c"