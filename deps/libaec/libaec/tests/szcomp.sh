#!/bin/sh
set -e
testfile=${srcdir}/../data/121B2TestData/ExtendedParameters/sar32bit.dat
if [ ! -f $testfile ]; then
    echo "ERROR: sample data not found."
    exit -1
fi
./check_szcomp $testfile
