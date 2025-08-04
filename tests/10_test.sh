#!/bin/bash

# Test that file is truncated when specified

EXP_CONTENT="bar foo"
TESTFILE=testfile.txt
echo "foo bar baz" > $TESTFILE

echo -n "$EXP_CONTENT" | zeugl -dt $TESTFILE
ACT_CONTENT=$(cat $TESTFILE)

if [ "$ACT_CONTENT" != "$EXP_CONTENT" ]; then
    echo "$(basename "$0"): error: Expected content $EXP_CONTENT, but got $ACT_CONTENT" 1>&2
    exit 1
fi
