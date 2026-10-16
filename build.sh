#!/usr/bin/env bash
# ============================================================
#  build.sh — Build APLIKASI PRODUKSI (setara build.bat, bisa
#  dipakai di Linux via mingw-w64 atau langsung di Windows/MSYS).
#
#  PEMISAH KETAT TES vs PRODUKSI:
#    * Mode produksi menyetel -DSCHEMATIC_PRODUCTION dan menolak
#      (-e) bila SCHEMATIC_TEST_STUBS ikut disetel.
#    * Tidak ada satu pun berkas dari tests/ yang dikompilasi ke
#      executable aplikasi.
# ============================================================
set -euo pipefail
cd "$(dirname "$0")"

CC_PROD="${CC_PROD:-i686-w64-mingw32-gcc}"

# --- Guard 1: mode tes tidak boleh bocor ke lingkungan build produksi ---
if [[ "${CPPFLAGS:-} ${CFLAGS:-}" == *SCHEMATIC_TEST_STUBS* ]]; then
    echo "[ERROR] CPPFLAGS/CFLAGS memuat SCHEMATIC_TEST_STUBS — itu mode TES, bukan produksi." >&2
    exit 1
fi

SRC_PROD=(
    src/main.c src/splash.c src/model.c src/fileio.c src/sim.c
    src/ui/draw.c src/ui/events.c
    src/ui/splash.c
    src/ui/toolbar_menubar.c src/ui/toolbar_symbols.c
    src/ui/toolbar_actions.c src/ui/toolbar_palette.c
)
while IFS= read -r f; do SRC_PROD+=("$f"); done < <(ls src/komponen/*.c | sort)

# --- Guard 2: daftar sumber produksi harus bebas dari prefix tests/ ---
for f in "${SRC_PROD[@]}"; do
    if [[ "$f" == tests/* ]]; then
        echo "[ERROR] Sumber produksi '$f' berasal dari folder tests/ — build ditolak." >&2
        exit 1
    fi
done

"$CC_PROD" -O2 -Wall -mwindows -municode \
    -DUNICODE -D_UNICODE -DSCHEMATIC_PRODUCTION=1 -Iinclude \
    "${SRC_PROD[@]}" \
    -o schematic_win.exe -lgdi32 -luser32 -lcomdlg32 -lm

echo "Build produksi berhasil: schematic_win.exe (tanpa berkas tests/)"
