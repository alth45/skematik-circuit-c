# Schematic-C — Editor Skematik Rangkaian (Win32 GDI)

Editor skematik sederhana berbasis **C + Win32 GDI**, tanpa framework tambahan.
Gambar simbol, geser (drag & drop), sambungkan dengan kabel, putar, simpan ke JSON,
lalu **Run** untuk simulasi: klik saklar ON/OFF dan lihat LED menyala.

![Platform](https://img.shields.io/badge/platform-Windows-blue)
![Language](https://img.shields.io/badge/language-C-orange)
![GUI](https://img.shields.io/badge/gui-Win32%20GDI-green)

---

## Fitur

| Fitur | Keterangan |
|---|---|
| Drag & Drop | Klik badan simbol, tahan, geser. Snap ke grid 20px. Kabel ikut tertarik. |
| Wiring antar pin | Tarik titik merah pin ke pin lain. Kabel siku + preview + highlight target. |
| Rotate (R) | Pilih simbol lalu tekan `R`. 0 -> 90 -> 180 -> 270. Pin & kabel ikut sudut. |
| Toolbar ikon | 16 tombol atas: Select, Wire, R, SW, BAT, LED, GND, CAP, TR, D, CP, Hapus, Clear, Save, Open, Run/Stop. |
| Komponen | Resistor, Saklar, Baterai, LED, Ground, Kapasitor, Transistor NPN, **Dioda**, **Kapasitor Polar** (baru). |
| Save / Open JSON | Dialog Windows, format UTF-8, status saklar ikut tersimpan. |
| Mode Run | Klik Run -> klik saklar ON/OFF -> LED menyala + kabel (+) merah + dioda forward (panah hijau) + deteksi korslet. |
| Shortcut | V/W/1-9, R, F5, Ctrl+S/O, Del, Esc. |
| Status bar | Pesan terakhir + counter komponen & kabel. |

---

## Tampilan

```
[Select][Wire][ R ][ SW ][BAT ][LED ][GND][CAP ][TR  ][ D ][ CP ][Hapus][Clear][Save][Open][Run]
tool aktif | F5: run/stop   R: rotate
+--------------------------------------------------+
|  (+)--[SW ON]--[ D ]--[LED nyala]--(-)           |
|        [ Rangkaian BENAR -- LED MENYALA ]        |
+--------------------------------------------------+
| RUN: saklar ON ...                4 komp, 5 kabel|
```

Warna saat Run: saklar ON = hijau (tuas datar), OFF = merah (tuas miring),
LED menyala = oranye + glow, dioda menghantar = panah arah arus hijau,
kabel (+) = merah tebal.
Banner: hijau benar, kuning belum lengkap, merah korsleting.

---

## Build & Jalankan

Butuh **MinGW-w64 gcc** di PATH (teruji gcc 15.1.0).

```bat
cd c:\Users\User\Downloads\schematic-c
build.bat
schematic_win.exe
```

Isi `build.bat`:

```bat
gcc -O2 -mwindows -municode -DUNICODE -D_UNICODE main.c model.c draw.c events.c toolbar.c fileio.c sim.c -o schematic_win.exe -lgdi32 -luser32 -lcomdlg32 -lm
```

> `schematic_win.c` monolitik lama sudah kedaluwarsa, yang dipakai file pecahan.

---

## Cara Pakai

### 1. Merakit (mode Edit)

1. Klik ikon komponen (mis. BAT), lalu klik canvas untuk menaruh.
2. Klik ikon Wire, tekan-tahan titik merah satu pin, tarik ke pin lain, lepas.
   Trik cepat: di mode Select langsung tarik pin -> pin.
3. Geser: mode Select -> tekan badan simbol -> tahan -> geser -> lepas.
4. Putar: klik simbol sampai kotak biru muncul -> tekan `R`.
5. Hapus: mode Hapus + klik, atau klik kanan, atau pilih + `Del`.

### 2. Contoh rangkaian LED menyala

Sambungkan loop tertutup:

```
BAT pin0 (+) -- kabel -- SW -- kabel -- R -- kabel -- LED pin0 (anoda)
                                                              |
BAT pin1 (-) -- kabel ---------------------------- LED pin1 (katoda)
```

Aturan pin: baterai kiri = (+) plat panjang, kanan = (-) plat pendek.
LED kiri = anoda, kanan = katoda. Ground = 1 pin, dihitung (-).
Dioda kiri = anoda (A), kanan = katoda (K) — menghantar searah A->K.
Kapasitor polar kiri = kutub (+), kanan = kutub (-); memblokir DC seperti kapasitor biasa.

### 2b. Contoh proteksi polaritas dengan dioda

```
BAT (+) -- kabel -- D (A->K) -- kabel -- LED pin0 (anoda)
BAT (-) -- kabel -------------------------- LED pin1 (katoda)
```

Saat RUN, dioda yang menghantar ditandai panah arah arus hijau.
Jika dioda terbalik (reverse-bias), rangkaian terbuka -> LED tetap mati.

### 3. Run simulasi

1. Klik tombol hijau **Run** (atau `F5`).
2. Klik **saklar** di canvas untuk ON/OFF. LED langsung update.
3. Baca banner tengah + status bar bawah.
4. Klik **Stop** (atau `F5`) untuk kembali edit.

Saat RUN dikunci: tambah/geser/putar/hapus, tarik kabel baru, ganti tool.

---
## Shortcut

| Tombol | Aksi |
|---|---|
| `V` | Tool Select / Geser |
| `W` | Tool Kabel / Wire |
| `1` `2` `3` `4` `5` | Resistor, Switch, Battery, LED, Ground |
| `6` `7` `8` `9` | Kapasitor, Transistor NPN, Dioda, Kapasitor Polar |
| `R` | Rotate 90 CW (dipilih / sedang di-drag) |
| `F5` / `F9` | Run / Stop simulasi |
| `Ctrl+S` | Save JSON |
| `Ctrl+O` | Open JSON |
| `Del` / `Backspace` | Hapus yang dipilih |
| `Esc` | Batal seleksi / wiring |

---

## Format JSON

UTF-8, contoh:

```json
{
  "app": "schematic-c",
  "version": 1,
  "components": [
    {"id": 1, "type": "battery", "x": 120, "y": 160, "rot": 0, "on": 0},
    {"id": 2, "type": "switch", "x": 220, "y": 160, "rot": 0, "on": 1},
    {"id": 3, "type": "led", "x": 340, "y": 160, "rot": 0, "on": 0}
  ],
  "wires": [
    {"a": {"id": 1, "pin": 0}, "b": {"id": 2, "pin": 0}},
    {"a": {"id": 2, "pin": 1}, "b": {"id": 3, "pin": 0}}
  ]
}
```

- `type`: `resistor | switch | battery | led | ground | capacitor | transistor | diode | capolar`
  (alias juga terbaca: `R/SW/BAT/LED/GND/CAP/TR`, `Diode/D`,
  `Capolar/capacitor_polar/electrolytic/CP`).
- `rot`: 0..3 (= 0/90/180/270). `on`: 0/1 khusus switch.
- `pin`: komponen 2-pin -> 0 kiri, 1 kanan (mengikuti rotasi); ground -> 0;
  transistor NPN (3-pin) -> 0 kolektor, 1 emitor, 2 basis.
- Batas: 256 komponen, 512 kabel, file maks 4 MB.
  Kabel yang id-nya tidak ada dibuang saat load.

---

## Aturan Simulasi (ideal)

Mesin `sim.c` memakai union-find, tiap pin = 1 node:

1. Gabungkan pin lewat kabel.
2. Resistor selalu tembus, saklar hanya jika ON.
   Baterai / LED / ground / kapasitor / kapasitor polar / transistor tidak tembus
   (arus lewat luar; kapasitor & kapasitor polar memblokir DC).
3. Net (+): semua pin0 baterai. Net (-): pin1 baterai + pin ground.
4. Dioda ideal: menghantar satu arah A->K hanya jika anoda (pin0) sudah di net (+);
   reverse-bias = open circuit. Status forward dipakai untuk panah hijau di simbol.
5. Korsleting jika (+) dan (-) satu net (termasuk lewat dioda forward).
6. LED nyala jika pin0 di (+) dan pin1 di (-) lewat jalur menghantar.
7. Kabel hot jika net-nya kena (+).

Pesan hasil:

- `Belum ada baterai — LED tidak bisa menyala.`
- `KORSLETING! (+) tersambung langsung ke (-).`
- `Rangkaian BENAR — LED MENYALA.`
- `Rangkaian belum lengkap — LED mati. Tutup saklar & sambungkan kabel.`

---

## Struktur Proyek

```
schematic-c/
  schematic.h  konstanta, struct, API semua modul (enum tipe komponen & tool)
  main.c       WndProc, toolbar_click, WM_PAINT, wWinMain
  model.c      state, pin/rotasi/hit-test, add/remove/wire, palet tombol toolbar
  draw.c       simbol GDI + rotasi (R/SW/BAT/LED/GND/CAP/TR/D/CP), pin, kabel hot, LED glow
  events.c     mouse (drag/wiring/toggle) + keyboard (1-9, R, F5, Ctrl+S/O, Del, Esc)
  toolbar.c    toolbar 16 ikon + hit-test + status
  fileio.c     save/load JSON + dialog + status
  sim.c        simulasi union-find + aturan dioda forward + pesan hasil
  build.bat    script build MinGW
  readme.md    file ini
```

Alur: events/toolbar -> model -> sim_recompute ->
draw/main baca sim_wire_hot / sim_comp_lit / sim_message.

---

## Troubleshooting

| Error | Solusi |
|---|---|
| `too few arguments to SetViewportOrgEx` | Pakai 4 argumen: `SetViewportOrgEx(hdc, x, y, NULL)`. |
| `undefined reference to draw_toolbar` | `toolbar.c` belum ikut compile atau korup. Pastikan build.bat memuat semua file. |
| `invalid storage class for function` | Fungsi tertulis di dalam fungsi lain. Pisahkan ke level file. |
| `continue/break not within loop` | Sisa parser di luar `while(1)`. Rapikan blok load JSON. |
| Tombol `R` tidak jalan | Klik canvas dulu agar fokus keyboard ke window. |
| LED tidak menyala | Cek baterai, loop tertutup, saklar ON (hijau), polaritas benar. |
| Dioda tidak menyalakan beban | Cek arah panah segitiga: anoda (A) harus di sisi (+). Balik posisi/rotasi (`R`) jika terpasang reverse-bias. |

---

## Ide Pengembangan

- Multi-select + copy/paste
- Undo/redo (Ctrl+Z/Y)
- Zoom & pan canvas
- Nilai komponen (Ohm/Volt) + hitung arus
- Export PNG / netlist SPICE
- Autosave + contoh `examples/*.json`
- Komponen baru: push button, buzzer, LDR, saklar SPDT, PNP

---

## Lisensi

Bebas dipakai untuk belajar / tugas. Sebut sumber jika disebar ulang.
