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
| Toolbar ikon | 17 tombol atas: Select, Wire, R, SW, BAT, AC, LED, GND, C, Q, D, CP, Hapus, Clear, Save, Open, Run/Stop. |
| Komponen | Resistor, Saklar, Baterai, LED, Ground, Kapasitor, Transistor NPN, Dioda, Kapasitor Polar, **Sumber AC** (baru). |
| Nilai komponen | Tiap komponen punya value (Ohm/V/VAC/Vf/F). Edit via dialog: double-klik atau klik-kanan -> *Edit Nilai*. Parser mendukung `2.2k`, `100u`, `47uF`, `2.2e3`. |
| Menu klik-kanan | Edit Nilai, Toggle saklar, Putar 90°, Duplikat, Informasi & Hasil Hitung (arus LED saat RUN), Hapus. Klik-kanan ganda = hapus. |
| Save / Open JSON | Dialog Windows, format UTF-8, status saklar & nilai komponen (`"value"`) ikut tersimpan. |
| Mode Run | Klik Run -> klik saklar ON/OFF -> LED menyala + kabel (+) merah + dioda forward (panah hijau) + deteksi korslet + hitung arus loop. |
| Analisis arus | Pesan RUN menampilkan I, R_loop, dan V sumber. LED kelebihan arus (>20 mA) memicu peringatan; terlalu besar -> "LED MELEDAK". |
| Shortcut | V/W/0-9, R, F5, Ctrl+S/O, Del, Esc. |
| Status bar | Pesan terakhir + counter komponen & kabel. |

---

## Tampilan

```
[Select][Wire][ R ][ SW ][BAT ][AC  ][LED ][GND][ C  ][ Q  ][ D ][ CP ][Hapus][Clear][Save][Open][Run]
tool aktif | F5: run/stop   R: rotate   double-klik: edit nilai
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

Struktur proyek memakai folder `include\` dan `src\` (lihat *Struktur Proyek*).
Butuh **MinGW-w64 gcc** di PATH.

```bat
cd c:\Users\User\Downloads\schematic-c
build.bat
schematic_win.exe
```

Isi `build.bat`:

```bat
gcc -O2 -Wall -mwindows -municode -DUNICODE -D_UNICODE -Iinclude ^
    src\main.c src\model.c src\fileio.c src\sim.c ^
    src\ui\draw.c src\ui\events.c src\ui\toolbar.c ^
    src\komponen\komponen_resistor.c src\komponen\komponen_switch.c ^
    src\komponen\komponen_battery.c src\komponen\komponen_led.c ^
    src\komponen\komponen_ground.c src\komponen\komponen_capacitor.c ^
    src\komponen\transistor.c src\komponen\komponen_diode.c ^
    src\komponen\komponen_capolar.c src\komponen\komponen_acsource.c ^
    -o schematic_win.exe -lgdi32 -luser32 -lcomdlg32 -lm
```

---

## Cara Pakai

### 1. Merakit (mode Edit)

1. Klik ikon komponen (mis. BAT), lalu klik canvas untuk menaruh.
2. Klik ikon Wire, tekan-tahan titik merah satu pin, tarik ke pin lain, lepas.
   Trik cepat: di mode Select langsung tarik pin -> pin.
3. Geser: mode Select -> tekan badan simbol -> tahan -> geser -> lepas.
4. Putar: klik simbol sampai kotak biru muncul -> tekan `R`.
5. Atur nilai: double-klik simbol (atau klik-kanan -> *Edit Nilai*...) —
   mis. resistor `2.2k`, baterai `9`, sumber AC `220` (VAC RMS), kapasitor `100u`.
6. Klik-kanan simbol: menu berisi Edit Nilai, Toggle saklar, Putar 90°,
   Duplikat, Informasi & Hasil Hitung, Hapus. Klik-kanan dua kali = hapus cepat.
7. Hapus: mode Hapus + klik, atau pilih + `Del`.

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

### 2c. Contoh sumber AC

Ganti BAT dengan ikon **AC** (shortcut `0`). Sumber AC: pin0 = Live, pin1 = Netral,
nilai default 220 VAC RMS @ 50 Hz.

```
AC Live -- kabel -- SW -- kabel -- R -- kabel -- LED anoda
AC Netral -- kabel ----------------------------- LED katoda
```

Jika rangkaian hanya disuplai sumber AC (tanpa baterai DC), simulator masuk
*mode AC*: kapasitor & kapasitor polar dianggap menghantar (impedansi AC),
dan hasil dihitung dengan pendekatan nilai RMS.

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
| `0` | Sumber AC |
| `R` | Rotate 90 CW (dipilih / sedang di-drag) |
| `F5` / `F9` | Run / Stop simulasi |
| `Ctrl+S` | Save JSON |
| `Ctrl+O` | Open JSON |
| `Del` / `Backspace` | Hapus yang dipilih |
| Double-klik | Edit nilai komponen |
| Klik-kanan | Menu konteks (nilai, toggle, putar, duplikat, info, hapus) |
| Esc | Batal seleksi / wiring |

---

## Format JSON

UTF-8, contoh:

```json
{
  "app": "schematic-c",
  "version": 1,
  "components": [
    {"id": 1, "type": "battery", "x": 120, "y": 160, "rot": 0, "on": 0, "value": 9},
    {"id": 2, "type": "switch", "x": 220, "y": 160, "rot": 0, "on": 1, "value": 0},
    {"id": 3, "type": "led", "x": 340, "y": 160, "rot": 0, "on": 0, "value": 2.0}
  ],
  "wires": [
    {"a": {"id": 1, "pin": 0}, "b": {"id": 2, "pin": 0}},
    {"a": {"id": 2, "pin": 1}, "b": {"id": 3, "pin": 0}}
  ]
}
```

- `type`: `resistor | switch | battery | led | ground | capacitor | transistor | diode | capolar | acsource`
  (alias juga terbaca: `R/SW/BAT/LED/GND/CAP/TR`, `Diode/D`,
  `Capolar/capacitor_polar/electrolytic/CP`,
  `ACSource/ac_source/ac/AC/SumberAC`).
- `value`: nilai komponen dalam satuan SI — resistor = Ohm, battery = V,
  acsource = VAC RMS, LED/dioda = Vf, kapasitor/capolar = Farad.
  Key lama `"v"` juga terbaca.
- `rot`: 0..3 (= 0/90/180/270). `on`: 0/1 khusus switch.
- `pin`: komponen 2-pin -> 0 kiri, 1 kanan (mengikuti rotasi); ground -> 0;
  transistor NPN (3-pin) -> 0 kolektor, 1 emitor, 2 basis;
  acsource -> 0 Live, 1 Netral.
- Batas: 256 komponen, 512 kabel, file maks 4 MB.
  Kabel yang id-nya tidak ada dibuang saat load.

---

## Aturan Simulasi (ideal)

Mesin `sim.c` memakai union-find, tiap pin = 1 node:

1. Gabungkan pin lewat kabel.
2. Resistor selalu tembus, saklar hanya jika ON.
   Baterai / LED / ground / kapasitor / kapasitor polar / transistor tidak tembus
   (arus lewat luar; kapasitor & kapasitor polar memblokir DC, TAPI menghantar
   pada mode AC murni).
3. Net (+): semua pin0 baterai + pin0 (Live) sumber AC.
   Net (-): pin1 baterai + pin1 (Netral) sumber AC + pin ground.
4. Dioda ideal: menghantar satu arah A->K hanya jika anoda (pin0) sudah di net (+);
   reverse-bias = open circuit. Status forward dipakai untuk panah hijau di simbol.
5. Korsleting jika (+) dan (-) satu net (termasuk lewat dioda forward).
6. LED nyala jika pin0 di (+) dan pin1 di (-) lewat jalur menghantar.
7. Kabel hot jika net-nya kena (+).
8. Mode AC: tanpa baterai DC tapi ada sumber AC -> loop dihitung dengan nilai RMS;
   kapasitor dianggap tembus impedansi AC.
9. Analisis arus loop (Ohm): I = (V_sumber - drop Vf) / R_serius. Arus > 20 mA
   memicu peringatan overcurrent; jauh di atas batas -> pesan "LED MELEDAK".

Pesan hasil:

- `Belum ada sumber (baterai/AC) — LED tidak bisa menyala.`
- `KORSLETING! (+) tersambung langsung ke (-).`
- `BAHAYA! LED MELEDAK — arus ... melebihi batas 20 mA. Tambah/perbesar resistor seri!`
- `PERINGATAN! LED rusak pelan — arus ... > 20 mA (max). Perbesar resistor seri.`
- `Rangkaian BENAR — LED MENYALA. I = ..., R_loop = ..., Vbat = ... V.`
- `Rangkaian AC BENAR — LED MENYALA (pendekatan RMS). I = ..., R_loop = ..., VAC = ... V.`
- `Rangkaian belum lengkap — LED mati. Tutup saklar & sambungkan kabel.`

---

## Struktur Proyek

```
schematic-c/
  include/
    schematic.h        konstanta, struct, API semua modul (enum tipe komponen & tool)
  src/
    main.c             WndProc, toolbar_click, WM_PAINT, wWinMain
    model.c            state, pin/rotasi/hit-test, add/remove/wire, nilai komponen
                       (parser/formatter/dialog edit value), palet tombol toolbar
    fileio.c           save/load JSON + dialog + status
    sim.c              simulasi union-find + dioda forward + mode AC + analisis arus
    ui/
      draw.c           dispatcher simbol + utilitas rotasi (with_rotation), label nilai,
                       pin, kabel hot/preview, canvas double-buffer
      events.c         mouse (drag/wiring/toggle/double-klik/klik-kanan) +
                       keyboard (0-9, R, F5, Ctrl+S/O, Del, Esc) + menu konteks
      toolbar.c        toolbar 17 ikon + hit-test + status
    komponen/
      komponen_resistor.c    simbol resistor (draw_k_resistor)
      komponen_switch.c      simbol saklar + status ON/OFF (draw_k_switch)
      komponen_battery.c     simbol baterai (draw_k_battery)
      komponen_led.c         simbol LED + glow saat nyala (draw_k_led)
      komponen_ground.c      simbol ground (draw_k_ground)
      komponen_capacitor.c   simbol kapasitor non-polar (draw_k_capacitor)
      transistor.c           simbol transistor NPN + panah emitor presisi (draw_k_transistor)
      komponen_diode.c       simbol dioda + panah arah alir (draw_k_diode)
      komponen_capolar.c     simbol kapasitor polar/elektrolit (draw_k_capolar)
      komponen_acsource.c    simbol sumber AC ~ (draw_k_acsource)
  build.bat            script build MinGW
  readme.md            file ini
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
| LED tidak menyala | Cek sumber (baterai/AC), loop tertutup, saklar ON (hijau), polaritas benar. |
| Dioda tidak menyalakan beban | Cek arah panah segitiga: anoda (A) harus di sisi (+). Balik posisi/rotasi (`R`) jika terpasang reverse-bias. |
| Pesan "LED MELEDAK" / overcurrent | Arus loop > 20 mA. Perbesar nilai resistor seri (double-klik resistor -> isi mis. `470`). |
| Sumber AC seperti open circuit | Normal untuk kapasitor pada DC; pastikan tidak ada baterai DC di canvas agar simulator masuk mode AC (kapasitor tembus). |

---

## Ide Pengembangan

- Multi-select + copy/paste
- Undo/redo (Ctrl+Z/Y)
- Zoom & pan canvas
- Export PNG / netlist SPICE
- Autosave + contoh `examples/*.json`
- Analisis AC penuh (reaktansi Xc, frekuensi) — saat ini masih pendekatan RMS
- Komponen baru: push button, buzzer, LDR, saklar SPDT, PNP

---

## Lisensi

Bebas dipakai untuk belajar / tugas. Sebut sumber jika disebar ulang.
