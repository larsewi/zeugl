#!/bin/bash

EXP_MODE=644
echo "Hello zeugl" | zeugl -d -c $EXP_MODE testfile.txt

ACT_MODE=$(stat testfile.txt --format %a)
if [ "$ACT_MODE" != "$EXP_MODE" ]; then
    echo "$(basename "$0"): error: Expected mode $EXP_MODE, but got $ACT_MODE" 1>&2
    exit 1
fi
