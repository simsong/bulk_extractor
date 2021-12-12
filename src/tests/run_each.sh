#!/bin/bash
# run each test once
tests=$(./test_be -l | egrep -v 'All available|test cases|\[')
for test in $tests ; do
    echo ========== $test ===========
    ./test_be $test
    echo
    echo
done
