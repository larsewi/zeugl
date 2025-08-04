#!/bin/bash

# Test original file is missing

TESTFILE=testfile.txt

echo "Hello zeugl" | zeugl -d $TESTFILE
