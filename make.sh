#!/bin/bash
echo Building PixelDbg

# Delete old binary
rm -f ./pixeldbg
rm -f ./pixeldbg64

args=("$@")
incl_dir=${args[0]}
lib_dir=${args[1]}

MACHINE_TYPE=`uname -m`
if [ ${MACHINE_TYPE} == 'x86_64' ]; then
  g++ main.cpp -D_FILE_OFFSET_BITS=64 -o pixeldbg64 -s -O3 -I$incl_dir -L$lib_dir -lfltk -lX11
else
  g++ main.cpp -o pixeldbg -s -O3 -I$incl_dir -L$lib_dir -lfltk -lX11
fi

if [ -f ./pixeldbg ]
then echo Everything Ok
fi
