#!/bin/bash

src_dir="$(dirname "$0")/.."
echo "Hello zeugl" | $src_dir/cli/zeugl -d -c 644 testfile.txt
stat testfile.txt

env

mode=$(stat testfile.txt --format %a)
if [ "$mode" != 644 ]; then
    echo "Error: Expected mode 644, but got $mode" 1>&2
    exit 1
fi
