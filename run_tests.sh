#!/usr/bin/env bash
# ============================================================
#  run_tests.sh — Build & jalankan SUITE UNIT TEST (mode TES).
#
#  PEMISAH KETAT TES vs PRODUKSI:
#    * Suite uji dikompilasi dengan -DSCHEMATIC_TEST_STUBS=1.
#      Header menolak kombinasi mode lewat #error, dan berkas
#      penunjang tes menolak kompilasi tanpa flag tersebut.
#    * Binary tes HANYA berisi tests/* + src/model.c + src/sim.c;
#      kode UI/Win32 produksi tidak ditautkan ke binary tes.
#    * Sebaliknya, build.bat / build.sh produksi TIDAK PERNAH
#      menyentuh folder tests/.
# ============================================================
set -euo pipefail
cd "$(dirname "$0")"

CC_TEST="${CC_TEST:-i686-w64-mingw32-gcc}"   # ganti gcc bila toolchain native tersedia
OUT="run_tests.exe"

SRC_TEST=(
    tests/test_main.c
    tests/test_model.c
    tests/test_sim.c
    tests/test_selection.c
    tests/test_support.c
    src/model.c
    src/sim.c
)

mkdir -p build_tmp

"$CC_TEST" -O1 -Wall -Wextra -Wno-unused-parameter -municode \
    -DUNICODE -D_UNICODE -DSCHEMATIC_TEST_STUBS=1 -Iinclude \
    "${SRC_TEST[@]}" \
    -o "$OUT" -lgdi32 -lm

echo "Build suite uji berhasil: $OUT"

# Jalankan bila bisa dieksekusi langsung (wine / MSYS); selain itu cukup biner.
if command -v wine >/dev/null 2>&1; then
    wine "./$OUT"
elif [[ "$(uname -s)" != Linux* ]]; then
    "./$OUT"
else
    echo "(Linux tanpa wine: binary tes sudah dibuat, jalankan di Windows/Wine.)"
fi
