@echo off
REM Build Schematic Editor (Win32 GDI) dengan MinGW gcc
REM draw.c = dispatcher; simbol tiap komponen di file sendiri:
REM   komponen_resistor.c komponen_switch.c komponen_battery.c komponen_led.c
REM   komponen_ground.c komponen_capacitor.c transistor.c komponen_diode.c komponen_capolar.c

gcc -O2 -mwindows -municode -DUNICODE -D_UNICODE main.c model.c draw.c ^
    komponen_resistor.c komponen_switch.c komponen_battery.c komponen_led.c ^
    komponen_ground.c komponen_capacitor.c transistor.c komponen_diode.c komponen_capolar.c ^
    events.c toolbar.c fileio.c sim.c -o schematic_win.exe -lgdi32 -luser32 -lcomdlg32 -lm

if %ERRORLEVEL% EQU 0 (
    echo Build berhasil: schematic_win.exe
) else (
    echo Build gagal.
)
