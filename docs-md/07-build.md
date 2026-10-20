## `docs/07-build.md`


# Build & Jalankan (Makefile)

[← Kembali ke readme](../readme.md)

Butuh **MinGW-w64 `gcc`** dan **`make`** di PATH.

```sh
make 
./schematic_win.exe
```

## Target

| Target | Kegunaan |
|---|---|
| `make` / `make app` | Build aplikasi produksi (`schematic_win.exe`, `-DSCHEMATIC_PRODUCTION=1`) |
| `make rebuild` | `clean` + build ulang aplikasi dari nol |
| `make tests` | Build binary tes (`run_tests.exe`, `-DSCHEMATIC_TEST_STUBS=1`) |
| `make test` | Build + jalankan binary tes |
| `make clean` | Hapus `.exe` / `.o` / `build_tmp/` |
| `make help` | Tampilkan bantuan target |

## Komposisi target produksi (`SRC_PROD` di `Makefile`)

```make
SRC_PROD := src/main.c src/splash.c src/model.c src/fileio.c src/sim.c src/oscilloscope.c \
	src/ui/draw.c src/ui/events.c \
	src\png_writer.c src\ui\export_png.c \
	src/ui/toolbar_menubar.c src/ui/toolbar_symbols.c \
	src/ui/toolbar_actions.c src/ui/toolbar_palette.c \
	src/ui/minimap.c \
	$(wildcard src/komponen/*.c)
```

## Ganti toolchain

```sh
make CC_PROD=i686-w64-mingw32-gcc
make CC_TEST=gcc
```

> ⚠️ **Toolbar overflow**: dengan 23 tombol palet + 4 aksi (27 total), pada
> jendela sempit tombol **Save / Open / Run** mungkin **terpotong** di kanan.
> Area komponen bisa di-scroll (wheel di toolbar); juga tersedia shortcut
> **F5** (Run), **Ctrl+S** (Save), **Ctrl+O** (Open), atau perlebar/maximize
> jendela.

## Suite uji

Dibangun terpisah oleh target `tests` / `test` (`-DSCHEMATIC_TEST_STUBS`);
folder `tests/` tidak pernah ikut ke executable produksi.

## Skrip lama

`build.bat(sudah dihapus)` / `build.sh(sudah dihapus)` / `run_tests.sh` masih ada sebagai referensi, tapi
**cara resmi sekarang adalah `Makefile`**.

---
