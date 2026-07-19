#!/bin/bash
# run each test once
TEST=test_be20_api
PATH=$PATH:.:..
tests=$($TEST -l | egrep -v 'All available|test cases|\[')
for test in $tests ; do
    echo ========== $test ===========
    echo '$' test_be $test
    $TEST $test
    echo
    echo
done
