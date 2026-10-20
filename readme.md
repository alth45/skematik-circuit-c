# Schematic-C — Editor Skematik Rangkaian (Win32 GDI)

Editor skematik berbasis **C + Win32 GDI**, tanpa framework tambahan.
Gambar simbol, geser (drag & drop), sambungkan dengan kabel, putar, simpan ke JSON,
lalu **Run** untuk simulasi: klik saklar ON/OFF, lihat LED menyala, ukur tegangan/arus,
dan **visualisasikan bentuk gelombang AC** lewat probe osiloskop.

![Platform](https://img.shields.io/badge/platform-Windows-blue)
![Language](https://img.shields.io/badge/language-C-orange)
![GUI](https://img.shields.io/badge/gui-Win32%20GDI-green)

> **📐 Catatan KaTeX**: Dokumen ini memakai [KaTeX](https://katex.org/) untuk rumus
> (`$...$` inline, `$$...$$` block). Gunakan viewer markdown yang mendukung KaTeX
> (VS Code + Markdown+Math, GitHub, GitLab, MkDocs, dll.) agar rumus tampil benar.

## Daftar Isi

| Dokumen | Isi |
|---|---|
| [docs/01-fitur.md](docs-md/01-fitur.md) | Fitur lengkap, tampilan ASCII, warna saat run |
| [docs/02-penggunaan.md](docs-md/02-penggunaan.md) | Cara pakai: merakit, rangkaian LED, AC, voltmeter, ammeter, probe, induktor, run |
| [docs/03-shortcut.md](docs-md/03-shortcut.md) | Tabel shortcut keyboard & mouse |
| [docs/04-format-json.md](docs-md/04-format-json.md) | Format file JSON (save/open) |
| [docs/05-osiloskop.md](docs-md/05-osiloskop.md) | Osiloskop: cara pakai, anatomi trace, rumus superposisi |
| [docs/06-simulasi.md](docs-md/06-simulasi.md) | Aturan simulasi (MNA-lite), rumus, akurasi |
| [docs/07-build.md](docs-md/07-build.md) | Build & jalankan (Makefile) |
| [docs/08-struktur-proyek.md](docs-md/08-struktur-proyek.md) | Struktur proyek & alur data |
| [docs/09-troubleshooting.md](docs-md/09-troubleshooting.md) | Daftar error & solusi |
| [docs/10-pengembangan.md](docs-md/10-pengembangan.md) | Ide pengembangan lanjutan |
| [docs/11-changelog.md](docs-md/11-changelog.md) | Ringkasan perubahan |

## Ringkasan Singkat

- **Fitur utama**: drag & drop, wiring antar pin, rotate, multi-select, undo/redo
  (depth 64), zoom & pan, toolbar 23 tombol, 19 komponen, save/open JSON,
  export PNG, minimap, mode run dengan deteksi korslet + simulasi LED + voltmeter
  + ammeter + probe osiloskop.
- **Platform**: Windows (Win32 GDI).
- **Bahasa**: C (MinGW-w64 `gcc`).
- **Build**: `make` / `make rebuild` / `make test`.

## Build Cepat

```sh
make
./schematic_win.exe
Butuh MinGW-w64 gcc dan make di PATH. Detail di
docs/07-build.md.
```

# Shortcut Populer Tombol	Aksi
```
V / B / W	Select / Blok / Wire
1–9, 0	Komponen (R, SW, BAT, LED, GND, C, Q, D, CP, AC)
L U M I P J Z F T	LDR, Buzzer, VM, AM, OSC, Induktor, Zener, MOSFET, POT
R	Rotate 90°
F5	Run/Stop
F12	Oscilloscope
Ctrl+Z / Ctrl+Y	Undo / Redo
Ctrl+S / Ctrl+O / Ctrl+E	Save / Open / Export PNG
Wheel / Middle-drag	Zoom / Pan
Daftar lengkap di docs/03-shortcut.md.
```
---
Lisensi
Lihat LICENCE (MIT).