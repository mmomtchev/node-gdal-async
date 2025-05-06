#!/bin/sh
set -e
if [ ! -f bench.rz ]; then
    echo "No encoded file found. Encoding now..."
    ${top_srcdir}/src/benc.sh ${top_srcdir}/data/typical.rz
fi
rm -f dec.dat
bsize=$(wc -c bench.dat | awk '{print $1}')
utime=$(./utime ./graec -d -n16 -j64 -r256 -m bench.rz dec.dat 2>&1)
perf=$(awk "BEGIN {print ${bsize}/1048576/${utime}}")
echo "[0;32m*** Decoding with $perf MiB/s user time ***[0m"
cmp bench.dat dec.dat
rm -f dec.dat
