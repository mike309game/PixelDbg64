#!/bin/bash
echo Building PixelDbg

# Delete old binary
rm -f ./pixeldbg

args=("$@")
incl_dir=${args[0]}
lib_dir=${args[1]}

g++ main.cpp -o pixeldbg -s -O3 -I$incl_dir -L$lib_dir -lfltk -lX11

if [ -f ./pixeldbg ]
then echo Everything Ok
fi
