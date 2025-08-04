#!/bin/bash

# Test original file is missing

INPUTFILE=input.txt
TESTFILE=testfile.txt

zeugl -d -c 644 -f $INPUTFILE $TESTFILE
