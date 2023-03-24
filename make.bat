echo off
cls

set arg1=%1
set arg2=%2
windres pdbg.rc -O coff -o pdbg.res
IF %PROCESSOR_ARCHITECTURE% == x86 (
g++ main.cpp -o PixelDbg.exe -mwindows -s -O3 -I%arg1% -Wint-to-pointer-cast -L%arg2% -lfltk -lgdi32 -lcomctl32 -lole32 -luuid -lcomdlg32 pdbg.res
)
IF %PROCESSOR_ARCHITECTURE% == AMD64 (
g++ main.cpp -o PixelDbg64.exe -mwindows -D_FILE_OFFSET_BITS=64 -s -O3 -I%arg1% -Wint-to-pointer-cast -L%arg2% -lfltk -lgdi32 -lcomctl32 -lole32 -luuid -lcomdlg32 pdbg.res
)