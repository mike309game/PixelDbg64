echo off
cls

set arg1=%1
set arg2=%2
windres pdbg.rc -O coff -o pdbg.res
g++ main.cpp -o PixelDbg.exe -march=i686 -mwindows -s -O3 -I%arg1% -Wint-to-pointer-cast -L%arg2% -lfltk -lgdi32 -lcomctl32 -lole32 -luuid -lcomdlg32 pdbg.res
