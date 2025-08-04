#!/bin/bash

# Test that file is created with correct permission mode bits when truncated

EXP_MODE=644
TESTFILE=testfile.txt

echo "Hello zeugl" | zeugl -dtc 644 $TESTFILE
ACT_MODE=$(stat $TESTFILE --format %a)

if [ "$ACT_MODE" != "$EXP_MODE" ]; then
    echo "$(basename "$0"): error: Expected mode $EXP_MODE, but got $ACT_MODE" 1>&2
    exit 1
fi
