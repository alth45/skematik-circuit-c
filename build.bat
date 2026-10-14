@echo off
REM Build Schematic Editor (Win32 GDI) dengan MinGW gcc

gcc -O2 -mwindows -municode -DUNICODE -D_UNICODE main.c model.c draw.c events.c toolbar.c fileio.c sim.c -o schematic_win.exe -lgdi32 -luser32 -lcomdlg32 -lm

if %ERRORLEVEL% EQU 0 (
    echo Build berhasil: schematic_win.exe
) else (
    echo Build gagal.
)
