@echo off
setlocal
REM ============================================================
REM  Build Schematic Editor (Win32 GDI) dengan MinGW gcc
REM  (versi: + Oscilloscope Probe)
REM ============================================================

where gcc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] gcc tidak ditemukan di PATH.
    exit /b 1
)

if defined SCHEMATIC_TEST_STUBS (
    echo [ERROR] SCHEMATIC_TEST_STUBS aktif — itu mode TES, bukan produksi.
    exit /b 1
)

gcc -O2 -Wall -mwindows -municode -DUNICODE -D_UNICODE -DSCHEMATIC_PRODUCTION=1 -Iinclude ^
    src\main.c src\splash.c src\model.c src\fileio.c src\sim.c src\oscilloscope.c ^
    src\ui\draw.c src\ui\events.c ^
    src\ui\toolbar_menubar.c src\ui\toolbar_symbols.c ^
    src\ui\toolbar_actions.c src\ui\toolbar_palette.c ^
    src\komponen\komponen_resistor.c src\komponen\komponen_switch.c ^
    src\komponen\komponen_battery.c src\komponen\komponen_led.c ^
    src\komponen\komponen_ground.c src\komponen\komponen_capacitor.c ^
    src\komponen\transistor.c src\komponen\komponen_diode.c ^
    src\komponen\komponen_inductor.c ^
    src\komponen\komponen_ammeter.c src\komponen\komponen_oscprobe.c ^
    src\komponen\komponen_capolar.c src\komponen\komponen_ldr.c ^
    src\komponen\komponen_buzzer.c src\komponen\komponen_acsource.c ^
    src\komponen\komponen_voltmeter.c ^
    -o schematic_win.exe -lgdi32 -luser32 -lcomdlg32 -lm

if %ERRORLEVEL% EQU 0 (
    echo Build berhasil: schematic_win.exe
) else (
    echo Build gagal.
    exit /b 1
)

endlocal