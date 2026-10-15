@echo off
setlocal
REM ============================================================
REM  Build Schematic Editor (Win32 GDI) dengan MinGW gcc
REM  Struktur proyek saat ini:
REM    include\schematic.h  - header bersama
REM    src\main.c src\model.c src\fileio.c src\sim.c
REM    src\ui\draw.c src\ui\events.c src\ui\toolbar_menubar.c
REM    src\ui\toolbar_symbols.c src\ui\toolbar_actions.c
REM    src\ui\toolbar_palette.c
REM    src\komponen\*.c     - 1 file per simbol komponen
REM                          (termasuk komponen_acsource.c = sumber AC)
REM
REM  PEMISAH KETAT TES vs PRODUKSI:
REM    * Build produksi TIDAK PERNAH menyertakan berkas dari folder tests\.
REM      (tests\stubs_production.c zaman dulu adalah satu-satunya pengecualian;
REM      sudah DIHAPUS karena implementasinya kini tinggal di src\sim.c.)
REM    * Mode produksi menyetel -DSCHEMATIC_PRODUCTION; bila seseorang ikut
REM      menyetel SCHEMATIC_TEST_STUBS, kompilasi gagal lewat #error di
REM      include\schematic.h — simbol tes mustahil bocor ke executable.
REM    * Suite uji dibangun terpisah oleh run_tests.sh (-DSCHEMATIC_TEST_STUBS).
REM ============================================================

where gcc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] gcc tidak ditemukan di PATH. Install MinGW-w64 lalu tambahkan ke PATH.
    exit /b 1
)

REM --- Guard: tolak build produksi bila mode TES ikut disetel di lingkungan ---
if defined SCHEMATIC_TEST_STUBS (
    echo [ERROR] SCHEMATIC_TEST_STUBS aktif di environment - itu mode TES, bukan produksi.
    echo         Jalankan "unset/buka cmd baru" atau pakai run_tests.sh untuk suite uji.
    exit /b 1
)

gcc -O2 -Wall -mwindows -municode -DUNICODE -D_UNICODE -DSCHEMATIC_PRODUCTION=1 -Iinclude ^
    src\main.c src\model.c src\fileio.c src\sim.c ^
    src\ui\draw.c src\ui\events.c ^
    src\ui\toolbar_menubar.c src\ui\toolbar_symbols.c ^
    src\ui\toolbar_actions.c src\ui\toolbar_palette.c ^
    src\komponen\komponen_resistor.c src\komponen\komponen_switch.c ^
    src\komponen\komponen_battery.c src\komponen\komponen_led.c ^
    src\komponen\komponen_ground.c src\komponen\komponen_capacitor.c ^
    src\komponen\transistor.c src\komponen\komponen_diode.c ^
    src\komponen\komponen_capolar.c src\komponen\komponen_ldr.c src\komponen\komponen_buzzer.c src\komponen\komponen_acsource.c ^
    -o schematic_win.exe -lgdi32 -luser32 -lcomdlg32 -lm

if %ERRORLEVEL% EQU 0 (
    echo Build berhasil: schematic_win.exe
) else (
    echo Build gagal.
    exit /b 1
)

endlocal
