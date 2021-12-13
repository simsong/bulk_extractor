#!/bin/bash
# run each test once
PATH=$PATH:.:..
tests=$(test_be -l | egrep -v 'All available|test cases|\[')
for test in $tests ; do
    echo ========== $test ===========
    echo '$' test_be $test
    test_be $test
    echo
    echo
done
