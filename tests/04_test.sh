#!/bin/bash

# Test that previous permission mode bits are respected

TESTFILE=testfile.txt
EXP_MODE=644

touch $TESTFILE
chmod $EXP_MODE $TESTFILE

echo "Hello zeugl" | zeugl -d -c 600 $TESTFILE

ACT_MODE=$(stat $TESTFILE --format %a)
if [ "$ACT_MODE" != "$EXP_MODE" ]; then
    echo "$(basename "$0"): error: Expected mode $EXP_MODE, but got $ACT_MODE" 1>&2
    exit 1
fi
