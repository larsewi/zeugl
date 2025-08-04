#!/bin/bash

# Test that file has expected content from input file

INPUTFILE=input.txt
TESTFILE=testfile.txt

echo "Hello zeugl" > $INPUTFILE
EXP_CONTENT=$(cat $INPUTFILE)

zeugl -d -c 644 -f $INPUTFILE $TESTFILE
ACT_CONTENT=$(cat $TESTFILE)

if [ "$ACT_CONTENT" != "$EXP_CONTENT" ]; then
    echo "$(basename "$0"): error: Expected content $EXP_CONTENT, but got $ACT_CONTENT" 1>&2
    exit 1
fi
