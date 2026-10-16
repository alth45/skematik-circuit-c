
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
| Toolbar ikon | 18 tombol palet + 4 tombol aksi: Select, Blok, Wire, R, SW, BAT, AC, LED, GND, C, Q, D, CP, LDR, BZR, VM, Hapus — lalu Clear, Save, Open, Run/Stop. |
| Komponen | Resistor, Saklar, Baterai, LED, Ground, Kapasitor, Transistor NPN, Dioda, Kapasitor Polar, LDR, Buzzer, Sumber AC, **Voltmeter**. |
| **Analisis nodal (MNA-lite)** | Mesin simulasi menyelesaikan **KCL** (`G·V = I`) dengan Gauss elimination + partial pivoting untuk tiap LED: hitung `V_thevenin` & `R_thevenin`, lalu `I = (Vth − Vf) / Rth`. Akurat untuk topologi **seri / paralel / pembagi tegangan** (presisi ~1% pada kasus tipikal). |
| Voltmeter | Mengukur **beda potensial (tegangan) antara dua titik** dalam rangkaian. Simbol lingkaran "V" dengan kutub (+)/(−); dipasang paralel; ideal (impedansi tak hingga) sehingga tidak mengubah rangkaian. Angka bacaan (mis. `9 V`) tampil di bawah simbol saat RUN dan di menu *Informasi & Hasil Hitung*. |
| Nilai komponen | Tiap komponen punya value (Ohm/V/VAC/Vf/F). Edit via dialog: double-klik atau klik-kanan -> *Edit Nilai*. Parser mendukung `2.2k`, `100u`, `47uF`, `2.2e3`. |
| Menu klik-kanan | Edit Nilai, Toggle saklar, Putar 90°, Duplikat, Informasi & Hasil Hitung (arus LED saat RUN), Hapus. Klik-kanan ganda = hapus. |
| Save / Open JSON | Dialog Windows, format UTF-8, status saklar & nilai komponen (`"value"`) ikut tersimpan. |
| Mode Run | Klik Run -> klik saklar ON/OFF -> LED menyala + kabel (+) merah + dioda forward (panah hijau) + deteksi korslet + hitung arus loop + angka bacaan voltmeter. |
| Analisis arus | Pesan RUN menampilkan I, R_loop, dan V sumber. LED kelebihan arus (>20 mA) memicu peringatan; terlalu besar -> "LED MELEDAK". |
| Shortcut | V/W/B, 0-9, L (LDR), U (Buzzer), M (Voltmeter), R, F5, Ctrl+S/O/A/D, Del, Esc. |
| Status bar | Pesan terakhir + counter komponen & kabel. |

---

## Tampilan

```
[Sel][Blk][Wir][ R ][ SW ][BAT ][AC  ][LED ][GND][ C  ][ Q  ][ D ][ CP ][LDR][BZR][ VM ][Hapus]
     [Clear][Save][Open][Run]
tool aktif | F5: run/stop   R: rotate   double-klik: edit nilai
+--------------------------------------------------+
|  (+)--[SW ON]--[ D ]--[LED nyala]--(-)           |
|          (V)  <- voltmeter paralel, bacaan "9 V" |
|        [ Rangkaian BENAR -- LED MENYALA ]        |
+--------------------------------------------------+
| RUN: saklar ON ...                5 komp, 6 kabel|
```

Warna saat Run: saklar ON = hijau (tuas datar), OFF = merah (tuas miring),
LED menyala = oranye + glow, dioda menghantar = panah arah arus hijau,
kabel (+) = merah tebal, angka bacaan voltmeter = hijau.
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

Isi `build.bat` (ringkas — lihat file untuk komentar lengkap):

```bat
gcc -O2 -Wall -mwindows -municode -DUNICODE -D_UNICODE -DSCHEMATIC_PRODUCTION=1 -Iinclude ^
    src\main.c src\model.c src\fileio.c src\sim.c ^
    src\ui\draw.c src\ui\events.c ^
    src\ui\toolbar_menubar.c src\ui\toolbar_symbols.c ^
    src\ui\toolbar_actions.c src\ui\toolbar_palette.c ^
    src\komponen\komponen_resistor.c src\komponen\komponen_switch.c ^
    src\komponen\komponen_battery.c src\komponen\komponen_led.c ^
    src\komponen\komponen_ground.c src\komponen\komponen_capacitor.c ^
    src\komponen\transistor.c src\komponen\komponen_diode.c ^
    src\komponen\komponen_capolar.c src\komponen\komponen_ldr.c ^
    src\komponen\komponen_buzzer.c src\komponen\komponen_acsource.c ^
    src\komponen\komponen_voltmeter.c ^
    -o schematic_win.exe -lgdi32 -luser32 -lcomdlg32 -lm
```

Build suite uji dilakukan terpisah oleh `run_tests.sh` (`-DSCHEMATIC_TEST_STUBS`);
folder `tests\` tidak pernah ikut ke executable produksi.

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
*mode AC*: kapasitor & kapasitor polar dianggap menghantar dengan **reaktansi
`Xc = 1/(2πfC)`** (bukan short ideal), sehingga arus AC terhitung realistis
(mis. C = 100 nF @ 50 Hz → Xc ≈ 31.8 kΩ).

### 2d. Contoh resistor paralel

Analisis nodal menangani **paralel** dengan benar:

```
BAT (+) --+-- R1=220Ω --+-- LED anoda
          |              |
          +-- R2=220Ω --+         LED katoda -- BAT (-)
```

R_eq = 110 Ω → I ≈ 63.6 mA (MELEDAK). Bukan 31.8 mA seperti bila hanya R1
yang dihitung — inilah peningkatan presisi dibanding versi path-based lama.

### 2e. Mengukur tegangan dengan Voltmeter

Voltmeter dipasang **paralel** — tidak memotong kabel, cukup tempelkan kedua
ujungnya ke dua titik yang ingin diukur beda potensialnya:

```
BAT (+) --+-- kabel -- R --+-- kabel -- LED anoda
          |                |
         (V) pin0 (+)   (V) pin1 (-)     <- voltmeter mengukur V pada R
          |                |
BAT (-) --+----------------+-- kabel -- LED katoda
```

1. Klik ikon **VM** di palet (atau tekan `M`), lalu klik canvas untuk menaruh.
2. Tarik kabel: pin **(+)** voltmeter ke titik A, pin **(−)** ke titik B.
3. Tekan **Run** (`F5`) — angka bacaan (mis. `4.5 V`, warna hijau) tampil di bawah
   simbol. Klik-kanan voltmeter -> *Informasi & Hasil Hitung*
   juga menampilkan bacaan tersebut.

Catatan perilaku:
- Voltmeter ideal: impedansi tak hingga -> **tidak menghantar arus**, jadi
  memasangnya tidak mengubah jalannya rangkaian.
- Bacaan diambil langsung dari **vektor potensial hasil analisis nodal** —
  akurat untuk pembagi tegangan, drop pada resistor, dan beda potensial antar
  node apa pun.
- Bacaan = potensial pin(+) − potensial pin(−). Kutub terbalik -> nilai negatif
  + keterangan `(kutub terbalik)` di menu info.
- Salah satu ujung mengambang (tidak terhubung) -> tampilan `-`.
- Nilai `v` voltmeter tidak dipakai (`comp_has_value(COMP_VOLTMETER) = 0`),
  sehingga dialog *Edit Nilai* tidak tersedia untuk alat ukur ini.

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
| `L` | LDR / Fotoresistor |
| `U` | Buzzer |
| `M` | Voltmeter |
| `B` | Blok / Multi-select |
| `Ctrl+A` | Pilih semua komponen |
| `Ctrl+D` | Duplikat seleksi/blok |
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

- `type`: `resistor | switch | battery | led | ground | capacitor | transistor | diode | capolar | acsource | voltmeter`
  (alias juga terbaca: `R/SW/BAT/LED/GND/CAP/TR`, `Diode/D`,
  `Capolar/capacitor_polar/electrolytic/CP`,
  `ACSource/ac_source/ac/AC/SumberAC`,
  `Voltmeter/VM`).
  Catatan: tipe `ldr` & `buzzer` **belum** didukung JSON — komponen bertipe itu
  dibuang saat save/load (lihat TODO `NOTE(komponen-ldr-buzzer)` di fileio.c).
- `value`: nilai komponen dalam satuan SI — resistor = Ohm, battery = V,
  acsource = VAC RMS, LED/dioda = Vf, kapasitor/capolar = Farad.
  Key lama `"v"` juga terbaca. Voltmeter tidak memakai `value` (selalu 0).
- `rot`: 0..3 (= 0/90/180/270). `on`: 0/1 khusus switch.
- `pin`: komponen 2-pin -> 0 kiri, 1 kanan (mengikuti rotasi); ground -> 0;
  transistor NPN (3-pin) -> 0 kolektor, 1 emitor, 2 basis;
  acsource -> 0 Live, 1 Netral.
- Batas: 256 komponen, 512 kabel, file maks 4 MB.
  Kabel yang id-nya tidak ada dibuang saat load.

---

## Aturan Simulasi (analisis nodal)

Mesin `sim.c` memakai **union-find** untuk kabel + penghantar ideal 0 Ω,
lalu **analisis nodal (MNA-lite)** untuk menyelesaikan tegangan/arus.

### Tahap 1 — Topologi (union-find)
1. Kabel menyatukan pin-pin menjadi satu net (root).
2. **Penghantar IDEAL 0 Ω** yang di-union hanya: saklar-ON, dioda forward,
   transistor-ON, kapasitor-mode-AC-murni. **Resistor, LDR, buzzer TIDAK
   di-union** — mereka tetap jadi *edge* berbobot R (kunci presisi).
3. Net (+): semua pin0 baterai + pin0 (Live) sumber AC.
   Net (−): pin1 baterai + pin1 (Netral) sumber AC + pin ground.

### Tahap 2 — Analisis nodal
4. Bangun matriks konduktansi `G` dan vektor arus `I` dari semua edge
   resistif (R, LDR, buzzer, kapasitor-AC dengan `Xc = 1/(2πfC)`).
5. **Sumber tegangan** → node dengan V **tetap** (baris constraint identitas),
   bukan MNA penuh. Propagasi rantai sumber seri otomatis.
6. Solve `G·V = I` dengan **Gauss elimination + partial pivoting**.
   Regularisasi `G_REG = 1e-12` mencegah matriks singular untuk node
   mengambang.

### Tahap 3 — Perhitungan LED (per LED, independen)
7. Untuk tiap LED:
   - **Vth** = V(anoda) − V(katoda) dari solve global (LED lain dianggap OPEN).
   - **Rth** = semua sumber di-short, injeksi +1 A di anoda & −1 A di katoda,
     solve ulang → `Rth = V(anoda) − V(katoda)`.
   - **I_LED** = `(Vth − Vf) / Rth` (nol bila Vth ≤ Vf).
8. Nasib LED:
   - `Rth ≈ 0` dan `Vth > Vf` → **MELEDAK** (short nyata).
   - `I > 5 × If_max (100 mA)` → **MELEDAK**.
   - `If_max < I ≤ 5 × If_max` → **overcurrent** (peringatan).
   - `I ≤ If_max` → **AMAN**.
   Toleransi relatif 1e-6 menjaga nilai ideal (mis. `(9−2)/350 = 20.000 mA`)
   selalu dinyatakan AMAN.

### Tahap 4 — Periferal
9. Korsleting: (+) dan (−) satu root (termasuk lewat dioda forward).
10. **Buzzer** ON bila `|V(pin0) − V(pin1)| > 1 V` (langsung dari solve nodal),
    atau bila seri/paralel dengan LED yang menyala.
11. **Voltmeter** ideal: kedua pin tidak pernah disatukan. Bacaan diambil
    langsung dari `V(pin0) − V(pin1)` hasil solve nodal — akurat untuk
    pembagi tegangan, drop antar node, dsb. Ujung mengambang → `-1` (UI `-`);
    kutub terbalik → nilai negatif.
12. Kabel hot: net yang memuat (+) sumber → merah tebal.

### Akurasi
- **Seri, paralel, campuran seri-paralel, pembagi tegangan**: presisi ~1%
  (batas praktis dari `%.4g` di pesan).
- **Model LED**: Vf konstan (bukan `Vf0 + Rs·I`); untuk arus 5–30 mA error
  Vf ±5% masih bisa terjadi di dunia nyata, namun klasifikasi aman/meledak
  berbasis datasheet (20 mA) tetap konservatif.
- **Transistor**: masih biner (ON/OFF berdasarkan bias basis-emitor), belum
  model β/Ib/Ic.
- **Dioda**: ideal satu arah, Vf = `c->v` (default 0.7 V) — belum
  Shockley-lite.

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
    sim.c              analisis nodal (MNA-lite, Gauss solver) + union-find untuk
                       penghantar ideal + perhitungan LED (Vth/Rth) + model Xc
                       kapasitor AC + voltmeter + buzzer + hot-wire
    ui/
      draw.c           dispatcher simbol + utilitas rotasi (with_rotation), label nilai,
                       pin, kabel hot/preview, canvas double-buffer
      events.c         mouse (drag/wiring/toggle/double-klik/klik-kanan) +
                       keyboard (0-9, R, F5, Ctrl+S/O, Del, Esc) + menu konteks
      toolbar_menubar.c  menu File/Edit/View/Simulasi/Bantu + popup + aksi
      toolbar_symbols.c  ikon mini tiap tool + shortname R/SW/BAT/AC/LED/...
      toolbar_actions.c  gambar tombol aksi Clear/Save/Open/Run
      toolbar_palette.c  hit-test + draw_toolbar strip tombol + status
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
      komponen_ldr.c         simbol LDR/fotoresistor + panah cahaya (draw_k_ldr)
      komponen_buzzer.c      simbol buzzer + label frekuensi (draw_k_buzzer)
      komponen_acsource.c    simbol sumber AC ~ (draw_k_acsource)
      komponen_voltmeter.c   simbol voltmeter (V) + angka bacaan tegangan saat RUN
                             (draw_k_voltmeter, bacaan via sim_voltmeter_reading)
  tests/                     suite uji (dibangun terpisah oleh run_tests.sh)
  build.bat            script build MinGW
  readme.md            file ini
```

Alur: events/toolbar -> model -> sim_recompute (union-find → analisis nodal → per-LED)
-> draw/main baca sim_wire_hot / sim_comp_lit / sim_message.

---

## Troubleshooting

| Error | Solusi |
|---|---|
| `warning: 'V_UNKNOWN' redefined` | Bentrok dengan `oleauto.h`. Ganti nama jadi `SIM_V_UNKNOWN` di sim.c. |
| `cannot open output file schematic_win.exe: Permission denied` | Aplikasi masih jalan. `taskkill /f /im schematic_win.exe` lalu rebuild. |
| `too few arguments to SetViewportOrgEx` | Pakai 4 argumen: `SetViewportOrgEx(hdc, x, y, NULL)`. |
| `undefined reference to draw_toolbar` | File `toolbar_*.c` belum ikut compile. Pastikan build.bat memuat semua file. |
| `invalid storage class for function` | Fungsi tertulis di dalam fungsi lain. Pisahkan ke level file. |
| `continue/break not within loop` | Sisa parser di luar `while(1)`. Rapikan blok load JSON. |
| Tombol `R` tidak jalan | Klik canvas dulu agar fokus keyboard ke window. |
| LED tidak menyala | Cek sumber (baterai/AC), loop tertutup, saklar ON (hijau), polaritas benar. |
| Dioda tidak menyalakan beban | Cek arah panah segitiga: anoda (A) harus di sisi (+). Balik posisi/rotasi (`R`) jika terpasang reverse-bias. |
| Pesan "LED MELEDAK" / overcurrent | Arus loop > 20 mA. Perbesar nilai resistor seri (double-klik resistor -> isi mis. `470`). |
| Sumber AC seperti open circuit | Normal untuk kapasitor pada DC; pastikan tidak ada baterai DC di canvas agar simulator masuk mode AC (kapasitor tembus dengan Xc). |
| Voltmeter selalu tampil "-" | Saat RUN, kedua pin harus terhubung ke net yang berbeda. Cek kabel: (+) ke titik A dan (-) ke titik B; satu ujung mengambang => bacaan "-". |
| Bacaan voltmeter 0 V padahal ada sumber | Kedua ujung tersambung ke net yang sama (mis. keduanya di sisi (+)). Pindahkan salah satu ujung melewati komponen (resistor/LED/dioda). |
| Hasil arus untuk paralel/seri janggal | Cek bahwa resistor tidak di-union sebagai short; hanya saklar-ON/dioda-fwd/transistor-ON/kapasitor-AC yang di-union. |
| Save lalu Open menghilangkan LDR/Buzzer | Memang belum didukung JSON (lihat Format JSON); simpan rangkaian tanpa LDR/Buzzer dulu. Voltmeter sudah tersimpan dengan benar. |

---

## Ide Pengembangan

- **LED Vf = Vf0 + Rs·I** (piecewise-linear) untuk presisi > 1% pada arus besar
- **Transistor model β/Ib/Ic** dengan Vbe ≈ 0.7 V, arus basis nyata
- **Dioda Shockley-lite**: `I = Is·(exp(V/nVt) − 1)` untuk Vf yang menyesuaikan arus
- Multi-select + copy/paste
- Undo/redo (Ctrl+Z/Y)
- Zoom & pan canvas
- Export PNG / netlist SPICE
- Autosave + contoh `examples/*.json`
- Komponen baru: push button, saklar SPDT, PNP, amperemeter (ukur arus seri),
  ohmmeter
- Dukungan JSON untuk LDR & Buzzer (saat ini masih dibuang saat save/load)

---

## Lisensi

Bebas dipakai untuk belajar / tugas. Sebut sumber jika disebar ulang.
```

---

## 📌 Ringkasan perubahan dari readme.md lama

| Bagian | Yang berubah |
|--------|--------------|
| **Fitur table** | Tambah baris **Analisis nodal (MNA-lite)** dengan penjelasan presisi 1%. |
| **Analisis arus row** | Masih sama, tapi sekarang didukung solver yang akurat. |
| **Bagian 2c (AC)** | Menyebut **Xc = 1/(2πfC)** eksplisit, bukan "dianggap tembus". |
| **Bagian 2d (baru)** | Contoh resistor paralel sebagai showcase presisi. |
| **Bagian 2e** | Voltmeter sekarang dijelaskan ambil bacaan dari **vektor potensial nodal**, bukan BFS head. |
| **Aturan Simulasi** | Ditulis ulang total: 4 tahap (union-find → nodal → per-LED → periferal) dengan detail solver, Vth/Rth, dan batas akurasi. |
| **Struktur Proyek** | Deskripsi `sim.c` diupdate (MNA + Gauss + Xc). |
| **Troubleshooting** | Tambah 3 baris baru: `V_UNKNOWN`, `Permission denied`, dan "hasil paralel janggal". |
| **Ide Pengembangan** | Hapus "Analisis AC penuh (reaktansi Xc)" (sudah diimplementasi), ganti dengan LED Vf dinamis, transistor β, dioda Shockley. |