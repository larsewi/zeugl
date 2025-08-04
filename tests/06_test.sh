#!/bin/bash

# Test that file has expected content from stdin

TESTFILE=testfile.txt
EXP_CONTENT="Hello zeugl"

echo $EXP_CONTENT | zeugl -d -c 644 $TESTFILE
ACT_CONTENT=$(cat $TESTFILE)

if [ "$ACT_CONTENT" != "$EXP_CONTENT" ]; then
    echo "$(basename "$0"): error: Expected content $EXP_CONTENT, but got $ACT_CONTENT" 1>&2
    exit 1
fi
