#!/bin/bash

# Test that permission mode bits on existing file are kept

TESTFILE=testfile.txt

echo -n foobar > $TESTFILE
echo -n baz | zeugl -da $TESTFILE

ACT_CONTENT=$(cat $TESTFILE)
EXP_CONTENT=foobarbaz

if [ "$ACT_CONTENT" != "$EXP_CONTENT" ]; then
    echo "$(basename "$0"): error: Expected content $EXP_MODE, but got $ACT_MODE" 1>&2
    exit 1
fi
