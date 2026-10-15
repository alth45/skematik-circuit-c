@echo off
setlocal
REM ============================================================
REM  Build Schematic Editor (Win32 GDI) dengan MinGW gcc
REM  Struktur proyek saat ini:
REM    include\schematic.h  - header bersama
REM    src\main.c src\model.c src\fileio.c src\sim.c
REM    src\ui\draw.c src\ui\events.c src\ui\toolbar.c
REM    src\komponen\*.c     - 1 file per simbol komponen
REM ============================================================

where gcc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] gcc tidak ditemukan di PATH. Install MinGW-w64 lalu tambahkan ke PATH.
    exit /b 1
)

gcc -O2 -Wall -mwindows -municode -DUNICODE -D_UNICODE -Iinclude ^
    src\main.c src\model.c src\fileio.c src\sim.c ^
    src\ui\draw.c src\ui\events.c src\ui\toolbar.c ^
    src\komponen\komponen_resistor.c src\komponen\komponen_switch.c ^
    src\komponen\komponen_battery.c src\komponen\komponen_led.c ^
    src\komponen\komponen_ground.c src\komponen\komponen_capacitor.c ^
    src\komponen\transistor.c src\komponen\komponen_diode.c ^
    src\komponen\komponen_capolar.c ^
    -o schematic_win.exe -lgdi32 -luser32 -lcomdlg32 -lm

if %ERRORLEVEL% EQU 0 (
    echo Build berhasil: schematic_win.exe
) else (
    echo Build gagal.
    exit /b 1
)

endlocal
