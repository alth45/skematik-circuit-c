# ============================================================
#  Makefile — Schematic Editor (Win32 GDI) + Unit Test Suite
#
#  Pemisah ketat TES vs PRODUKSI (seperti build.bat / build.sh):
#    * Target produksi (app)  : -DSCHEMATIC_PRODUCTION=1,
#      TIDAK BOLEH ada SCHEMATIC_TEST_STUBS, TIDAK PERNAH
#      menyentuh folder tests/.
#    * Target tes (test/tests): -DSCHEMATIC_TEST_STUBS=1,
#      HANYA tests/* + src/model.c + src/sim.c (tanpa UI Win32).
#
#  Cara pakai (kamu build sendiri):
#    make              -> build aplikasi (schematic_win.exe)
#    make app          -> sama dengan di atas
#    make tests        -> build binary tes (run_tests.exe)
#    make test         -> build + jalankan binary tes
#    make clean        -> hapus .exe + .o + build_tmp/
#    make help         -> tampilkan bantuan ini
#    make CC_PROD=x86_64-w64-mingw32-gcc   -> ganti compiler produksi
#    make CC_TEST=gcc                      -> ganti compiler tes
# ============================================================

# --- Toolchain (bisa di-override dari command line / env) ---
CC      ?= gcc
CC_PROD ?= $(CC)
CC_TEST ?= $(CC)

# --- Output ---
APP      := schematic_win.exe
TEST_BIN := run_tests.exe

# --- Flags ---
CFLAGS_PROD  := -O2 -Wall -mwindows -municode -DUNICODE -D_UNICODE -DSCHEMATIC_PRODUCTION=1 -Iinclude
LDFLAGS_PROD := -lgdi32 -luser32 -lcomdlg32 -lm

CFLAGS_TEST  := -O1 -Wall -Wextra -Wno-unused-parameter -municode -DUNICODE -D_UNICODE -DSCHEMATIC_TEST_STUBS=1 -Iinclude
LDFLAGS_TEST := -lgdi32 -lm

# --- Sumber PRODUKSI (disamakan dengan build.bat) ---
# Pakai wildcard untuk src/komponen/*.c supaya komponen baru otomatis ikut.
SRC_PROD := src/main.c src/splash.c src/model.c src/fileio.c src/sim.c src/oscilloscope.c \
	src/ui/draw.c src/ui/events.c \
	src/ui/toolbar_menubar.c src/ui/toolbar_symbols.c \
	src/ui/toolbar_actions.c src/ui/toolbar_palette.c \
	$(wildcard src/komponen/*.c)

# --- Sumber TES (disamakan dengan run_tests.sh) ---
SRC_TEST := tests/test_main.c tests/test_model.c tests/test_sim.c \
	tests/test_selection.c tests/test_support.c \
	src/model.c src/sim.c

# ============================================================
#  Target default = aplikasi produksi
# ============================================================
.PHONY: all app rebuild tests test clean help check-prod-guard

all: app

app: check-prod-guard $(APP)

rebuild: clean app

$(APP): $(SRC_PROD)
	$(CC_PROD) $(CFLAGS_PROD) $(SRC_PROD) -o $(APP) $(LDFLAGS_PROD)
	@echo "Build produksi berhasil: $(APP) (tanpa berkas tests/)"

# Guard: mode tes tidak boleh bocor ke build produksi.
# (cek level make, bukan shell — jalan di cmd.exe maupun sh)
ifneq (,$(findstring SCHEMATIC_TEST_STUBS,$(CPPFLAGS) $(CFLAGS)))
$(error [ERROR] CPPFLAGS/CFLAGS memuat SCHEMATIC_TEST_STUBS — itu mode TES, bukan produksi)
endif

check-prod-guard:
	@echo "[check] mode produksi OK (tanpa SCHEMATIC_TEST_STUBS)"

# ============================================================
#  Suite unit test
# ============================================================
tests: $(TEST_BIN)

$(TEST_BIN): $(SRC_TEST)
	$(CC_TEST) $(CFLAGS_TEST) $(SRC_TEST) -o $(TEST_BIN) $(LDFLAGS_TEST)
	@echo "Build suite uji berhasil: $(TEST_BIN)"

test: $(TEST_BIN)
	-./$(TEST_BIN)
	-$(TEST_BIN)

# ============================================================
#  Bersih-bersih (pilih perintah sesuai OS)
# ============================================================
ifeq ($(OS),Windows_NT)
clean:
	-del /Q $(APP) $(TEST_BIN) 2>NUL
	-del /Q *.o 2>NUL
	-del /Q src\*.o src\ui\*.o src\komponen\*.o tests\*.o 2>NUL
	-if exist build_tmp rmdir /S /Q build_tmp
else
clean:
	rm -f $(APP) $(TEST_BIN) *.o src/*.o src/ui/*.o src/komponen/*.o tests/*.o
	rm -rf build_tmp
endif

help:
	@echo "Target yang tersedia:"
	@echo "  make / make app   - build aplikasi produksi: $(APP)"
	@echo "  make rebuild      - clean + build ulang aplikasi"
	@echo "  make tests        - build binary tes: $(TEST_BIN)"
	@echo "  make test         - build + jalankan binary tes"
	@echo "  make clean        - hapus hasil build"
	@echo "  make help         - tampilkan bantuan ini"
	@echo ""
	@echo "Override toolchain, contoh:"
	@echo "  make CC_PROD=i686-w64-mingw32-gcc"
	@echo "  make CC_TEST=gcc"
