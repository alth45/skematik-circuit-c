
# Schematic-C — Editor Skematik Rangkaian (Win32 GDI)

Editor skematik berbasis **C + Win32 GDI**, tanpa framework tambahan.
Gambar simbol, geser (drag & drop), sambungkan dengan kabel, putar, simpan ke JSON,
lalu **Run** untuk simulasi: klik saklar ON/OFF, lihat LED menyala, ukur tegangan/arus,
dan **visualisasikan bentuk gelombang AC** lewat probe osiloskop.

![Platform](https://img.shields.io/badge/platform-Windows-blue)
![Language](https://img.shields.io/badge/language-C-orange)
![GUI](https://img.shields.io/badge/gui-Win32%20GDI-green)

---

## Fitur

| Fitur | Keterangan |
|---|---|
| Drag & Drop | Klik badan simbol, tahan, geser. Snap ke grid 20px. Kabel ikut tertarik. |
| Wiring antar pin | Tarik titik merah pin ke pin lain. Kabel siku + preview + highlight target. |
| Rotate (R) | Pilih simbol lalu tekan `R`. 0 → 90 → 180 → 270. Pin & kabel ikut sudut. |
| **Multi-select / blok** | Drag di area kosong untuk memblok banyak komponen; drag grup terpilih untuk memindahkan satu kesatuan; `Ctrl+D` menduplikat blok + kabel internalnya. |
| **Undo / Redo** | `Ctrl+Z` / `Ctrl+Y` atau Edit > Undo/Redo (depth 64). Mencakup tambah, geser/drag, putar, hapus, kabel, edit nilai, toggle saklar/LDR, waveform AC, duplikat & clear. Klik saklar saat RUN tidak di-undo. |
| **Zoom & Pan canvas** | Wheel = zoom di kursor, `Ctrl++`/`Ctrl+-` = zoom di tengah, `Ctrl+0` = zoom-fit, **mouse tengah + drag** = pan. Rentang 25 %–800 %. Status bar menampilkan `zoom XX%`. |
| Toolbar ikon | **23 tombol palet** (Select, Blok, Wire, Hapus + 19 komponen: R, SW, BAT, AC, LED, GND, C, Q, D, CP, LDR, BZR, VM, AM, OSC, L, Zener, MOSFET, POT — area komponen bisa di-scroll) + 4 tombol aksi: Clear, Save, Open, Run/Stop. |
| Komponen | Resistor, Saklar, Baterai, Sumber AC, LED, Ground, Kapasitor, Transistor NPN, Dioda, Kapasitor Polar, LDR, Buzzer, Voltmeter, Ammeter, **Probe Osiloskop**, **Induktor**, **Dioda Zener**, **MOSFET N-Channel**, **Potensiometer**. |
| **Dioda Zener** | Pin A–K, arah sama dengan dioda biasa. **Forward**: menghantar seperti dioda. **Reverse**: breakdown bila `V_K − V_A > Vz` (default 5,1 V, edit via *Edit Nilai*, satuan `Vz`). Status forward/breakdown tampil saat RUN. |
| **MOSFET N-Channel** | 3 pin (gate, source, drain), model biner seperti transistor: **ON** (D–S di-union 0 Ω) bila gate tersambung ke net (+) dan source ke net (−). Nilai = `Vth` (default 2 V). Saat RUN simbol hijau + label ON/OFF. |
| **Potensiometer** | 3 pin: A (kiri), B (kanan), W/wiper (atas-tengah). Dimodelkan sebagai **dua resistor seri**: `R_A = R·pos%` (A–W) dan `R_B = R·(100−pos)%` (W–B), clamp minimum 1 mΩ. Nilai = R total (default **10 kΩ**); posisi wiper 0–100 % (default 50 %) via klik-kanan → *Posisi Wiper* (preset 0/10/25/50/75/90/100 %). |
| **Analisis nodal (MNA-lite)** | Mesin simulasi menyelesaikan **KCL** (`G·V = I`) dengan Gauss elimination + partial pivoting. LED dihitung dengan V_thevenin & R_thevenin lalu `I = (Vth − Vf) / Rth`. Akurat untuk seri/paralel/pembagi tegangan (~1 % pada kasus tipikal). |
| **Voltmeter** | Ukur beda potensial dua titik, dipasang **paralel**, ideal (impedansi tak hingga → tidak mengubah rangkaian). Angka bacaan tampil di bawah simbol saat RUN. |
| **Ammeter** | Ukur arus lewat jalur, dipasang **seri**, shunt internal 0.1 Ω. Angka bacaan `A` tampil di bawah simbol saat RUN. |
| **Probe Osiloskop** | Lihat **bentuk gelombang AC terhadap waktu**. Maksimal 4 kanal, warna merah/hijau/biru/oranye. Panel terpisah via **View → Oscilloscope (F12)**; refresh otomatis 5 Hz. Lihat **§ Osiloskop** di bawah. |
| **Induktor** | Simbol 4 lengkung. **DC**: short ideal (union seperti saklar-ON). **AC**: reaktansi `XL = 2πfL`. Default 100 µH. |
| **Bentuk gelombang AC** | Klik-kanan Sumber AC → *Bentuk Gelombang* → Sinus / Kotak / Segitiga. Sumber AC default 220 VAC RMS @ 50 Hz; peak = √2 · Vrms. |
| **Splash screen** | Split view: brand & judul di kiri, grid 12 simbol komponen di kanan, progress bar di bawah. |
| Nilai komponen | Edit via dialog: double-klik atau klik-kanan → *Edit Nilai*. Parser mendukung `2.2k`, `100u`, `47uF`, `2.2e3`. Induktor pakai satuan `H`, Zener `Vz`, MOSFET `Vth`, Potensiometer `Ω` (R total). |
| **Save / Open JSON** | UTF-8, format stabil. Semua komponen ikut tersimpan (termasuk `wave` sumber AC, status Terang/Gelap LDR, `zener`, `mosfet`, `potentiometer` dengan posisi wiper di field `on`). |
| **Export PNG** | **File → Export as PNG (`Ctrl+E`)**: render ulang canvas (grid + kabel + simbol + banner RUN) ke PNG 24-bit lewat dialog Save-As. Overlay UI (kotak seleksi, marquee, minimap) tidak ikut. |
| **Minimap** | Overview seluruh canvas di pojok kanan-bawah (**View → Minimap**, default ON). Kotak merah = viewport saat ini; klik/drag di minimap = pan canvas ke titik itu. Otomatis tersembunyi bila canvas kosong. |
| Mode Run | Klik Run → klik saklar ON/OFF → LED menyala + kabel (+) merah + dioda forward (panah hijau) + deteksi korslet + hitung arus loop + bacaan voltmeter/ammeter. |
| **About dialog** | Menu **Bantu → Tentang Schematic Editor** menampilkan ringkasan fitur & versi. |
| Shortcut | V/W/B, 0–9, J (Induktor), L (LDR), M (Voltmeter), I (Ammeter), P (OSC probe), Z (Zener), F (MOSFET), T (Potensiometer), R, F5, F12, Ctrl+S/O/A/D/E, Ctrl+Z/Y, Ctrl+/−/0, Del, Esc. |
| Status bar | Pesan terakhir + counter komponen & kabel + zoom level. |

---

## Tampilan

```
[Sel][Blk][Wir][ R ][ SW ][BAT ][AC ][LED][GND][ C ][ Q ][ D ][ CP][LDR][BZR][ VM][ AM][OSC][ L ][ZEN][FET][POT][Hapus]
     [Clear][Save][Open][Run]
tool aktif | F5: run/stop   R: rotate   Ctrl +/-/0: zoom   Ctrl+Z/Y: undo/redo   double-klik: edit nilai
+------------------------------------------------------------+
|  (+)--[SW ON]--[ R ]--[LED nyala]--(-)                      |
|         |   (V)  <- voltmeter paralel, bacaan "6.5 V"       |
|         |   (AMMETER) <- ammeter seri, bacaan "31.8 mA"     |
|         |   (OSC) <- probe osc di titik node, trace sinus   |
|   [ Rangkaian BENAR -- LED MENYALA ]                        |
+------------------------------------------------------------+
| RUN: saklar ON ...               9 komp, 8 kabel | zoom 150%|
```

**Warna saat Run:**
- Saklar ON = hijau (tuas datar), OFF = merah (tuas miring)
- LED menyala = oranye + glow; **LED meledak** = merah tebal + label nilai merah
- Dioda menghantar = panah arah arus hijau
- Kabel (+) = merah tebal
- Bacaan voltmeter & ammeter = hijau
- Banner: hijau benar, kuning belum lengkap, merah korslet

**Oscilloscope window** (View → Oscilloscope, F12):
- Panel gelap dengan grid 10×8 + subdivision, sumbu tengah menyala hijau.
- Trace tiap probe (maks 4) berwarna merah/hijau/biru/oranye.
- Skala vertikal otomatis; horizontal 4 periode @ 50 Hz (= 80 ms).

---

## Osiloskop (Probe + Window)

### Cara pakai cepat

1. Klik ikon **OSC** di toolbar (atau tekan `P`), klik canvas untuk menaruh probe.
2. Sambungkan **pin kiri** probe (sinyal / tip) ke titik yang ingin diukur, dan **pin kanan** (klip referensi) ke titik referensi (biasanya ground / netral).
3. Tekan **F12** atau **View → Oscilloscope** → jendela oscilloscope terbuka.
4. Trace muncul & auto-refresh ~5 Hz. Ubah rangkaian di canvas (saklar, waveform, nilai komponen) — trace ikut update.

### Anatomi trace

- Timebase tetap **80 ms** (4 periode @ 50 Hz) → 8 ms per division.
- Skala vertikal **otomatis** — dipilih sebagai pangkat dua terkecil yang masih memuat semua trace.
- Nilai yang ditampilkan: `V_probe(t) = V_dc + Σ_k  gain_k · V_ac_k(t)`
  - `V_dc` = respon saat semua sumber AC dimatikan (hanya sumber DC).
  - `gain_k` = V_probe saat sumber AC-ke-k diset 1 V (unit), sisanya 0.
  - `V_ac_k(t)` = gelombang instan sumber AC (sinus / kotak / segitiga).
- Cara ini murah dan **eksak** untuk rangkaian linear (superposisi), tanpa transient solver.

### Batasan

- Frekuensi tetap **50 Hz** (sama seperti model AC).
- Hanya resistor & sumber AC yang dimodelkan dalam respon frekuensi. Kapasitor/induktor **ikut** karena diperlakukan sebagai edge reaktif (`Xc`, `XL`).
- Semua sumber AC diasumsikan **sefasa**. Phase offset per-source belum ada.

---

## Build & Jalankan (Makefile)

Butuh **MinGW-w64 `gcc`** dan **`make`** di PATH.

```sh
make
./schematic_win.exe
```

Target yang tersedia:

| Target | Kegunaan |
|---|---|
| `make` / `make app` | Build aplikasi produksi (`schematic_win.exe`, `-DSCHEMATIC_PRODUCTION=1`) |
| `make rebuild` | `clean` + build ulang aplikasi dari nol |
| `make tests` | Build binary tes (`run_tests.exe`, `-DSCHEMATIC_TEST_STUBS=1`) |
| `make test` | Build + jalankan binary tes |
| `make clean` | Hapus `.exe` / `.o` / `build_tmp/` |
| `make help` | Tampilkan bantuan target |

Komposisi target produksi (`SRC_PROD` di `Makefile`):

```make
SRC_PROD := src/main.c src/splash.c src/model.c src/fileio.c src/sim.c src/oscilloscope.c \
	src/ui/draw.c src/ui/events.c \
	src\png_writer.c src\ui\export_png.c \
	src/ui/toolbar_menubar.c src/ui/toolbar_symbols.c \
	src/ui/toolbar_actions.c src/ui/toolbar_palette.c \
	src/ui/minimap.c \
	$(wildcard src/komponen/*.c)
```

Ganti toolchain bila perlu:

```sh
make CC_PROD=i686-w64-mingw32-gcc
make CC_TEST=gcc
```

> ⚠️ **Toolbar overflow**: dengan 23 tombol palet + 4 aksi (27 total), pada jendela sempit tombol **Save / Open / Run** mungkin **terpotong** di kanan. Area komponen bisa di-scroll (wheel di toolbar); juga tersedia shortcut **F5** (Run), **Ctrl+S** (Save), **Ctrl+O** (Open), atau perlebar/maximize jendela.

Suite uji dibangun terpisah oleh target `tests` / `test` (`-DSCHEMATIC_TEST_STUBS`); folder `tests/` tidak pernah ikut ke executable produksi.

Skrip lama `build.bat` / `build.sh` / `run_tests.sh` masih ada sebagai referensi, tapi **cara resmi sekarang adalah `Makefile`**.

---

## Cara Pakai

### 1. Merakit (mode Edit)

1. Klik ikon komponen (mis. BAT), lalu klik canvas untuk menaruh.
2. Klik ikon Wire, tekan-tahan titik merah satu pin, tarik ke pin lain, lepas.
   Trik cepat: di mode Select langsung tarik pin → pin.
3. Geser: mode Select → tekan badan simbol → tahan → geser → lepas.
4. Putar: klik simbol sampai kotak putus-putus muncul → tekan `R`.
5. Atur nilai: double-klik simbol (atau klik-kanan → *Edit Nilai*…)
6. Klik-kanan simbol: menu konteks (Edit Nilai, Toggle saklar, Putar, Duplikat, **Bentuk Gelombang** untuk AC, **Buka Osiloskop** untuk probe, Info, Hapus).
7. Zoom / pan: **wheel** = zoom di kursor; **Ctrl++/−** = zoom di tengah; **Ctrl+0** = zoom fit; **mouse tengah + drag** = pan.

### 2. Rangkaian LED menyala

```
BAT pin0 (+) -- kabel -- SW -- kabel -- R -- kabel -- LED pin0 (anoda)
                                                              |
BAT pin1 (-) -- kabel ---------------------------- LED pin1 (katoda)
```

Aturan pin:
- Baterai: kiri = (+), kanan = (−).
- Sumber AC: pin0 = Live, pin1 = Netral.
- LED: kiri = anoda, kanan = katoda.
- Dioda: kiri = anoda (A), kanan = katoda (K).
- Kapasitor polar: kiri = (+), kanan = (−).
- Ground: 1 pin, dihitung (−).
- **Transistor NPN**: pin0 = Kolektor (kanan-atas), pin1 = Emitor (kanan-bawah), pin2 = Basis (kiri-tengah). **Simbol NPN standar**: base di kiri, C & E di kanan, panah keluar di emitor.

### 2b. Proteksi polaritas dengan dioda

```
BAT (+) -- kabel -- D (A→K) -- kabel -- LED pin0
BAT (-) -- kabel -------------------------- LED pin1
```

Saat RUN, dioda yang forward ditandai panah arah arus hijau. Jika reverse-bias, loop terbuka.

### 2c. Sumber AC + kapasitor

Ganti BAT dengan ikon **AC** (shortcut `0`). Sumber AC: pin0 = Live, pin1 = Netral,
default 220 VAC RMS @ 50 Hz.

```
AC Live   -- kabel -- SW -- kabel -- R -- kabel -- LED anoda
AC Netral -- kabel ----------------------------- LED katoda
```

Jika hanya sumber AC (tanpa baterai), simulator masuk **mode AC**:
- Kapasitor → reaktansi `Xc = 1/(2πfC)` (bukan short).
- **Induktor → reaktansi `XL = 2πfL`** (bukan short).

### 2d. Bentuk gelombang AC (sinus / kotak / segitiga)

Klik-kanan Sumber AC → **Bentuk Gelombang** → pilih Sinus / Kotak / Segitiga.
Bentuk gelombang ini hanya memengaruhi **probe osiloskop** (simulasi DC & LED tetap
memakai pendekatan RMS).

### 2e. Voltmeter

Voltmeter dipasang **paralel** — tempelkan kedua ujungnya di dua titik yang
ingin diukur beda potensialnya:

```
BAT (+) --+-- kabel -- R --+-- kabel -- LED anoda
          |                |
         (V) pin0 (+)   (V) pin1 (−)     <- voltmeter mengukur V pada R
          |                |
BAT (-) --+----------------+-- kabel -- LED katoda
```

- Klik **VM** (`M`), taruh, tarik kabel dari pin (+) ke titik A & pin (−) ke titik B.
- Tekan **F5** → bacaan (mis. `4.5 V`, hijau) tampil di bawah simbol.
- Voltmeter ideal (impedansi tak hingga → tidak menghantar arus).
- Bacaan = `V(pin+) − V(pin−)`. Kutub terbalik → negatif + `(kutub terbalik)` di info.
- Salah satu ujung mengambang → `-`.

### 2f. Ammeter

Ammeter dipasang **seri** — potong jalur dan masukkan ammeter di antaranya:

```
BAT (+) -- (AM) pin0 (+) -- R -- LED -- BAT (−)
              └─ pin1 (−) keluar jalur
```

- Klik **AM** (`I`), taruh di antara dua komponen seri.
- Tekan **F5** → bacaan (mis. `31.8 mA`, hijau) tampil di bawah simbol.
- Shunt internal 0.1 Ω — drop kecil (< 5 mV pada 30 mA), tidak mengubah arus
  secara signifikan tapi cukup untuk menghitung `I = ΔV / 0.1` dengan stabil.

### 2g. Probe osiloskop

```
AC Live   -- R (220 Ω) --+-- [OSC pin0]
                         |
                    node yang ingin diukur
                         |
AC Netral --+------------+-- [OSC pin1]
```

- Klik **OSC** (`P`), taruh, tarik kabel pin kiri ke titik ukur, pin kanan ke
  referensi (biasanya netral/ground).
- Tekan **F12** atau **View → Oscilloscope** → jendela oscilloscope terbuka.
- Trace muncul otomatis (refresh 5 Hz). Klik-kanan AC source → ubah waveform
  untuk melihat respons yang berbeda.

### 2h. Induktor

- Klik **L** (`J`), taruh di jalur.
- **DC (baterai)**: induktor short ideal. Arus mengalir seperti tanpa komponen.
- **AC (sumber AC)**: reaktansi `XL = 2π · 50 · L` Ω.
  - `L = 100 µH` → `XL ≈ 0.03 Ω` (hampir short).
  - `L = 100 mH` → `XL ≈ 31 Ω`.
  - `L = 1 H` → `XL ≈ 314 Ω` (LED meredup/hampir mati).
- Double-klik untuk edit nilai (`100u`, `10m`, `1`, dst).

### 3. Run simulasi

1. Klik tombol hijau **Run** (atau `F5`).
2. Klik **saklar** di canvas untuk ON/OFF. LED langsung update.
3. Baca banner tengah + status bar bawah.
4. Klik **Stop** (atau `F5`) untuk kembali edit.

Saat RUN dikunci: tambah/geser/putar/hapus, tarik kabel baru, ganti tool, dan zoom/pan juga dikunci.

---

## Shortcut

| Tombol | Aksi |
|---|---|
| `V` | Tool Select / Geser |
| `B` | Tool Blok / Multi-select |
| `W` | Tool Kabel / Wire |
| `1` `2` `3` `4` `5` | Resistor, Switch, Battery, LED, Ground |
| `6` `7` `8` `9` | Kapasitor, Transistor NPN, Dioda, Kapasitor Polar |
| `0` | Sumber AC |
| `L` | LDR / Fotoresistor |
| `U` | Buzzer |
| `M` | Voltmeter |
| `I` | Ammeter |
| `P` | Probe Osiloskop |
| `J` | Induktor |
| `Z` | Dioda Zener |
| `F` | MOSFET N-Channel |
| `T` | Potensiometer |
| `Ctrl+A` | Pilih semua komponen |
| `Ctrl+D` | Duplikat seleksi/blok |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `R` | Rotate 90° CW (dipilih / sedang di-drag) |
| `F5` | Run / Stop simulasi |
| `F9` | Hitung ulang simulasi (Run) |
| `F12` | Buka / fokus jendela **Oscilloscope** |
| `Ctrl+` `+` / `Ctrl+` `−` | Zoom in / out di tengah canvas |
| `Ctrl+0` | Zoom fit (semua komponen muat) |
| **Mouse wheel** | Zoom di kursor |
| **Mouse tengah + drag** | Pan canvas |
| `Ctrl+S` | Save JSON |
| `Ctrl+O` | Open JSON |
| `Ctrl+E` | Export canvas sebagai PNG |
| `Del` / `Backspace` | Hapus yang dipilih |
| Double-klik | Edit nilai komponen |
| Klik-kanan | Menu konteks (nilai, toggle, putar, duplikat, waveform, buka osc, info, hapus) |
| `Esc` | Batal seleksi / wiring |

---

## Format JSON

UTF-8, contoh (rangkaian RC–LED dengan probe):

```json
{
  "app": "schematic-c",
  "version": 1,
  "components": [
    {"id": 1, "type": "acsource", "x": 120, "y": 160, "rot": 0, "on": 0, "wave": 0, "value": 220},
    {"id": 2, "type": "resistor", "x": 220, "y": 160, "rot": 0, "on": 0, "wave": 0, "value": 220},
    {"id": 3, "type": "inductor", "x": 320, "y": 160, "rot": 0, "on": 0, "wave": 0, "value": 0.1},
    {"id": 4, "type": "led",      "x": 420, "y": 160, "rot": 0, "on": 0, "wave": 0, "value": 2.0},
    {"id": 5, "type": "oscprobe", "x": 320, "y": 240, "rot": 0, "on": 0, "wave": 0, "value": 0},
    {"id": 6, "type": "ammeter",  "x": 370, "y": 160, "rot": 0, "on": 0, "wave": 0, "value": 0},
    {"id": 7, "type": "ldr",      "x": 220, "y": 260, "rot": 0, "on": 1, "wave": 0, "value": 1000000},
    {"id": 8, "type": "buzzer",   "x": 420, "y": 260, "rot": 0, "on": 0, "wave": 0, "value": 1000}
  ],
  "wires": [
    {"a": {"id": 1, "pin": 0}, "b": {"id": 2, "pin": 0}},
    {"a": {"id": 2, "pin": 1}, "b": {"id": 3, "pin": 0}},
    {"a": {"id": 3, "pin": 1}, "b": {"id": 4, "pin": 0}}
  ]
}
```

- **`type`**: `resistor | switch | battery | led | ground | capacitor | transistor | diode | capolar | acsource | voltmeter | ammeter | ldr | buzzer | oscprobe | inductor | zener | mosfet | potentiometer`.
  - Alias juga diterima: `R/SW/BAT/LED/GND/CAP/TR`, `Diode/D`, `Capolar/capacitor_polar/electrolytic/CP`,
    `ACSource/ac_source/ac/AC/SumberAC`, `Voltmeter/VM`, `Ammeter/AM`,
    `LDR/photoresistor`, `Buzzer/BZR`, `OSCProbe/OSC`, `Inductor/IND/L`,
    `Zener/Z/ZD/zenerdiode`, `MOSFET/nmos/NMOS/FET`, `POT/trimmer/rheostat`.
- **`value`** (satuan SI):
  - resistor = Ω, battery = V, acsource = VAC RMS, LED/dioda = Vf (V),
    kapasitor/capolar = F, **ldr = R_gelap (Ω)**, **buzzer = frekuensi (Hz)**,
    **inductor = H**, **zener = Vz (V)**, **mosfet = Vth (V)**, **potentiometer = R total (Ω)**.
  - **Potentiometer**: posisi wiper 0–100 disimpan di field **`on`** (bukan `value`).
  - Voltmeter & Ammeter tidak memakai `value` (selalu 0).
  - Key lama `"v"` juga terbaca.
- **`wave`** (khusus `acsource`): 0 = sinus (default), 1 = kotak, 2 = segitiga.
  Diabaikan untuk tipe lain.
- **`on`**: umumnya 0/1.
  - Switch: 1 = tertutup, 0 = terbuka.
  - **LDR**: 1 = TERANG (R efektif ~1 % dari nilai gelap), 0 = GELAP.
  - **Potentiometer**: posisi wiper **0–100** (persen; di-load di-clamp ke rentang ini).
  - Tipe lain: diabaikan.
- **`rot`**: 0..3 (= 0/90/180/270).
- **`pin`**: 2-pin → 0 = kiri, 1 = kanan (mengikuti rotasi); ground → 0;
  transistor NPN → 0 = Kolektor, 1 = Emitor, 2 = Basis; MOSFET → 3 pin (gate/source/drain);
  potensiometer → 0 = A, 1 = B, 2 = W (wiper);
  acsource → 0 = Live, 1 = Netral.
- Batas: 256 komponen, 512 kabel, file maks 4 MB. Kabel yang id-nya tidak ada dibuang saat load.

---

## Aturan Simulasi (analisis nodal)

Mesin `sim.c` memakai **union-find** untuk kabel + penghantar ideal 0 Ω,
lalu **analisis nodal (MNA-lite)** untuk menyelesaikan tegangan/arus.

### Tahap 1 — Topologi (union-find)
1. Kabel menyatukan pin-pin menjadi satu net (root).
2. **Penghantar IDEAL 0 Ω** yang di-union hanya: saklar-ON, **induktor saat mode DC**, dioda/Zener forward, transistor-ON / **MOSFET-ON**.
   - Resistor, LDR, buzzer, ammeter **tidak** di-union — tetap edge berbobot R.
   - Kapasitor / induktor dalam mode AC juga tidak di-union — jadi edge reaktif.
   - **Zener reverse**: bukan union — dimodelkan sebagai sumber tegangan `Vz` (K = "+"), status breakdown dicari iteratif (maks. 20 iterasi) sampai stabil.
3. Net (+): semua pin0 baterai + pin0 (Live) sumber AC. Net (−): pin1 sumber + pin ground.

### Tahap 2 — Analisis nodal
4. Matriks konduktansi `G` dibangun dari semua edge resistif:
   - Resistor → `R = c->v`
   - LDR → `R_gelap × 1 %` saat Terang, atau `R_gelap` saat Gelap
   - Buzzer → `R = 100 Ω` (tipikal)
   - Ammeter → `R = 0.1 Ω`
   - Kapasitor (AC) → `Xc = 1/(2πfC)`
   - Induktor (AC) → `XL = 2πfL`
   - **Potensiometer** → dua edge seri: A–W `R·pos%`, W–B `R·(100−pos)%` (clamp min 1 mΩ)
5. Sumber tegangan → node dengan V **tetap** (baris constraint identitas).
6. Solve `G·V = I` dengan **Gauss + partial pivoting**. Regularisasi `1e-12` cegah singular.

### Tahap 3 — Perhitungan LED
7. Per LED: hitung `Vth` (V anoda − V katoda dari solve global) dan `Rth` (semua sumber di-short, injeksi 1 A, solve ulang). `I = (Vth − Vf) / Rth`.
8. Klasifikasi:
   - `Rth ≈ 0` & `Vth > Vf` → **MELEDAK**.
   - `I > 100 mA` → **MELEDAK**.
   - `20 mA < I ≤ 100 mA` → **overcurrent** (peringatan).
   - `I ≤ 20 mA` → **AMAN**.

### Tahap 4 — Periferal
9. Korsleting: (+) & (−) satu root (termasuk lewat dioda forward).
10. **Buzzer** ON bila `|V(pin0) − V(pin1)| > 1 V`, atau seri/paralel dengan LED yang menyala.
11. **Voltmeter** ideal → baca `V(pin0) − V(pin1)`. Kedua pin tidak pernah di-union.
12. **Ammeter** → baca `|V(pin0) − V(pin1)| / 0.1`.
13. **Probe OSC** → baca `V_dc + Σ gain_k · V_ac_k(t)` via superposisi (dihitung lazy setelah `sim_recompute`).
14. Kabel hot: net yang memuat (+) sumber → merah tebal.

### Bentuk gelombang AC
- Peak `Vpk = √2 · Vrms`.
- `WAVE_SINE` → `Vpk · sin(2πft)`.
- `WAVE_SQUARE` → `+Vpk` di paruh periode positif, `−Vpk` di negatif.
- `WAVE_TRIANGLE` → naik linear 0..Vpk..0..−Vpk dalam satu periode.

### Akurasi
- **Seri, paralel, campuran, pembagi tegangan**: ~1 % (batas praktis `%.4g`).
- **Model LED**: Vf konstan.
- **Transistor**: biner (ON/OFF berdasarkan bias basis-emitor), belum model β.
- **MOSFET**: biner seperti transistor — ON bila gate di net (+) & source di net (−), D–S short 0 Ω; belum model linier/ohmic.
- **Dioda**: ideal satu arah, Vf = `c->v` (default 0.7 V).
- **Zener**: forward ideal; reverse breakdown dikunci di tepat `Vz` (default 5.1 V), tanpa resistansi dinamis.
- **Potensiometer**: model resistif linear murni (dua edge seri, tanpa clamp non-linear); akurat sesuai pembagian `pos%`.
- **Induktor**: DC = short ideal, AC = XL linier.
- **Probe OSC**: eksak via superposisi (linear).

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
    main.c             WndProc, toolbar_click, WM_PAINT (zoom/pan, banner, status), render_canvas_scene,
                       accelerator (Ctrl+N/O/S/E, F5/F9/F12), wWinMain
    splash.c           splash split-view 720×400 (brand kiri, 12 simbol kanan)
    model.c            state, pin/rotasi/hit-test, add/remove/wire, nilai komponen,
                       parser/formatter/dialog edit value, zoom & pan API, palet tombol,
                       undo/redo stack (snapshot bounded, batch)
    fileio.c           save/load JSON + dialog + export PNG dialog + status
                       (termasuk ldr/buzzer/oscprobe/inductor/ammeter/zener/mosfet/potentiometer)
    sim.c              analisis nodal (MNA + Gauss) + union-find + perhitungan LED (Vth/Rth)
                       + Xc kapasitor + XL induktor + voltmeter + ammeter + buzzer
                       + hot-wire + probe osiloskop (sim_ac_voltage, sim_probe_voltage)
                       + zener forward/breakdown iteratif + MOSFET-ON
    oscilloscope.c     jendela oscilloscope: 4 kanal, refresh 5 Hz, auto skala
    png_writer.c       penulis PNG 24-bit (encoder deflate sederhana, tanpa libpng)
    ui/
      draw.c           dispatcher simbol + utilitas rotasi (with_rotation, komposisi zoom)
                       + label nilai + pin + kabel hot/preview + overlay seleksi + marquee
      events.c         mouse (drag/wiring/toggle/double-klik/klik-kanan/minimap) +
                       keyboard (0-9, J, I, P, L, M, U, R, Z, F, T, F5, F12, Ctrl+S/O/A/D/E/Z/Y,
                                 Ctrl +/−/0, Del, Esc) + menu konteks + waveform picker
                       + preset wiper potensiometer
      minimap.c        overview canvas + viewport merah + klik/drag pan (View > Minimap)
      export_png.c     render canvas bersih (tanpa overlay) ke RGB buffer → png_writer
      toolbar_menubar.c  menu File/Edit/View/Simulasi/Bantu + popup + aksi + About dialog
      toolbar_symbols.c  ikon mini tiap tool + shortname R/SW/BAT/AC/LED/…/OSC/L/Zener/MOSFET/POT
      toolbar_actions.c  gambar tombol aksi Clear/Save/Open/Run
      toolbar_palette.c  hit-test + draw_toolbar strip tombol + status
    komponen/
      komponen_resistor.c    simbol resistor
      komponen_switch.c      simbol saklar + status ON/OFF
      komponen_battery.c     simbol baterai
      komponen_led.c         simbol LED + glow saat nyala
      komponen_ground.c      simbol ground
      komponen_capacitor.c   simbol kapasitor non-polar
      transistor.c           simbol transistor NPN (B kiri, C & E kanan) + panah emitor
      komponen_diode.c       simbol dioda + panah arah alir
      komponen_capolar.c     simbol kapasitor polar/elektrolit
      komponen_ldr.c         simbol LDR/fotoresistor + panah cahaya
      komponen_buzzer.c      simbol buzzer + label frekuensi
      komponen_acsource.c    simbol sumber AC ~ (gelombang sinus)
      komponen_voltmeter.c   simbol voltmeter (V) + angka bacaan tegangan saat RUN
      komponen_ammeter.c     simbol ammeter (A) + angka bacaan arus saat RUN
      komponen_oscprobe.c    simbol probe osiloskop (tip + badan + klip)
      komponen_inductor.c    simbol induktor (4 lengkung)
      komponen_zener.c       simbol dioda zener (balik + Z) + status fwd/breakdown
      komponen_mosfet.c      simbol MOSFET N-channel enhancement + label ON/OFF
      komponen_pot.c         simbol potensiometer IEC + panah wiper posisi %
  tests/                 suite uji (target `make tests` / `make test`): model, sim,
                         selection, undo
  docs/                  dokumentasi HTML API (index per modul + halaman per fungsi;
                         dibuat/machine-update via upd_docs.py)
  Makefile               build resmi: app + tests + clean + rebuild
  build.bat / build.sh / run_tests.sh   skrip lama, hanya referensi
  readme.md              file ini
```

Alur: `events/toolbar` → `model` → `sim_recompute` (union-find → analisis nodal → per-LED → periferal) → `draw/main` baca `sim_wire_hot` / `sim_comp_lit` / `sim_voltmeter_reading` / `sim_ammeter_reading` / `sim_probe_voltage` / `sim_message`.

`oscilloscope.c` meng-query `sim_probe_voltage` tiap 200 ms untuk trace.

---

## Troubleshooting

| Error | Solusi |
|---|---|
| `warning: 'V_UNKNOWN' redefined` | Bentrok dengan `oleauto.h`. Ganti nama jadi `SIM_V_UNKNOWN` di sim.c. |
| `cannot open output file schematic_win.exe: Permission denied` | Aplikasi masih jalan. `taskkill /f /im schematic_win.exe` lalu rebuild. |
| `undefined reference to osc_open / osc_refresh` | `oscilloscope.c` belum masuk `SRC_PROD` di `Makefile`. |
| `undefined reference to sim_ammeter_connected` | Definisikan di `sim.c` + deklarasikan di `schematic.h`. |
| `multiple definition of sim_recompute` | `model.c` dan `sim.c` tertukar isinya. Kembalikan `model.c` ke versi aslinya. |
| `too few arguments to SetViewportOrgEx` | Pakai 4 argumen: `SetViewportOrgEx(hdc, x, y, NULL)`. |
| `undefined reference to draw_toolbar` | File `toolbar_*.c` belum ikut compile. |
| Tombol `R` tidak jalan | Klik canvas dulu agar fokus keyboard ke window. |
| LED tidak menyala | Cek sumber, loop tertutup, saklar ON (hijau), polaritas benar. |
| Dioda tidak menyalakan beban | Arah panah segitiga: anoda (A) di sisi (+). Balik posisi/rotasi (`R`). |
| Pesan "LED MELEDAK" / overcurrent | Arus loop > 20 mA. Perbesar resistor seri (double-klik resistor → `470`). |
| Voltmeter selalu tampil "-" | Saat RUN, kedua pin harus terhubung ke net berbeda. Cek kabel (+)/(−). |
| Bacaan voltmeter 0 V padahal ada sumber | Kedua ujung tersambung ke net yang sama. Pindahkan satu ujung melewati komponen. |
| Ammeter selalu tampil "-" | Ammeter harus dipasang **seri** (pin0 → pin1 satu jalur). Kalau dipasang paralel, arus tidak terdefinisi. |
| Trace osiloskop kosong | Pastikan minimal 1 komponen **OSC probe** ada di canvas, tekan F12, dan probe terkoneksi ke dua net berbeda. |
| Trace osiloskop datar di angka tertentu | Tidak ada sumber AC → hanya DC. Tambahkan Sumber AC. Atau probe kedua ujungnya di net yang sama (beda = 0). |
| Bentuk gelombang tidak berubah saat ganti waveform | Ubah waveform **saat RUN** atau ganti sambil oscilloscope terbuka — refresh 5 Hz otomatis. Kalau perlu refresh instan: tutup & buka lagi F12. |
| Induktor seperti short di AC | `XL = 2πfL`; untuk `L = 100 µH` di 50 Hz memang ~0.03 Ω (mendekati short). Perbesar nilai (`1`, `10m`, dst). |
| Rotasi simbol "pecah" (simbol keluar dari kotak seleksi / pin lepas) | Fitur zoom + rotasi harus dikomposisi di `with_rotation`. Pastikan `draw.c` memakai versi terbaru (yang menyimpan transform parent via `GetWorldTransform` dan memanggil `SetWorldTransform(&prev)` setelah `RestoreDC`). |
| Tombol Save/Open/Run di toolbar tidak terlihat | Toolbar overflow. Gunakan `Ctrl+S`, `Ctrl+O`, `F5`, atau maximize jendela. |
| Save lalu Open menghilangkan LDR/Buzzer | Sudah diperbaiki. Kalau masih hilang, cek versi `fileio.c` — pastikan `comp_type_name`/`comp_type_from_name` sudah mencakup `"ldr"` & `"buzzer"`. |

---

## Ide Pengembangan

- **Transient solver** (`dI/dt`, `dV/dt`) — induktor & kapasitor akan punya bentuk gelombang RC/RL yang eksak, bukan hanya steady-state.
- **Phase offset per-sumber AC** (mis. 0°, 90°, 180°) — memungkinkan simulasi rangkaian 3-fasa sederhana.
- **Frekuensi per-sumber AC** (bukan hanya 50 Hz global).
- **Skala timebase & voltase oscilloscope manual** (dropdown 1× / 10× / auto).
- **Freeze / capture** trace oscilloscope (pause tombol di panel).
- **Export PNG dari oscilloscope** (canvas sudah bisa via `Ctrl+E`).
- **Paste dari clipboard** (Ctrl+C / Ctrl+V) untuk blok terpilih.
- **Model transistor β/Ib/Ic** dengan Vbe ≈ 0.7 V.
- **Dioda Shockley-lite** (Vf menyesuaikan arus) untuk presisi > 1 %.
- **Model MOSFET linier/ohmic** (Rds(on), region aktif) — saat ini masih biner.
- **LED Vf dinamis** (Vf0 + Rs·I).
- **Net label** / port untuk multi-sheet.
- **Autosave** + folder `examples/*.json`.
- **Komponen tambahan**: push-button, saklar SPDT, PNP, IC op-amp sederhana, IC 555.

---

## Lisensi

Bebas dipakai untuk belajar / tugas. Sebut sumber jika disebar ulang.

Lihat [LICENCE](LICENCE) (MIT).


## Ringkasan perubahan dibanding readme.md lama

| Bagian | Yang berubah |
|---|---|
| **Fitur table** | Baris **Undo/Redo** (Ctrl+Z/Y, depth 64), **Dioda Zener**, **MOSFET N-Channel**, **Potensiometer**, **Export PNG**, **Minimap** ditambahkan. Toolbar kini **23 tombol palet**; daftar komponen & nilai (Zener `Vz`, MOSFET `Vth`, Pot `Ω` + wiper) diperbarui. |
| **Tampilan (ASCII)** | Baris toolbar 23 tombol (termasuk ZEN/FET/POT); status bar `zoom XX%`. |
| **Shortcut** | `Z` (Zener), `F` (MOSFET), `T` (Potensiometer), `Ctrl+Z/Y` (undo/redo), `Ctrl+E` (export PNG), `I`, `P`, `J`, `F12`, `Ctrl +/-/0`, mouse wheel, mouse tengah. |
| **Format JSON** | `type` baru: `zener | mosfet | potentiometer` (+ alias `Z/ZD/zenerdiode`, `nmos/FET`, `pot/trimmer/rheostat`). `value` = Vz (V) / Vth (V) / R total (Ω); wiper pot di field `on`. |
| **Aturan Simulasi** | Tahap 1 menyebut dioda/**Zener forward** + **MOSFET-ON**; Zener reverse = sumber `Vz` iteratif. Tahap 2: edge **potensiometer** (A–W / W–B). Akurasi mencantumkan batas model Zener, MOSFET biner & POT linear. |
| **Struktur Proyek** | `png_writer.c`, `ui/export_png.c`, `ui/minimap.c`, `komponen_zener.c`, `komponen_mosfet.c`, `komponen_pot.c`; `model.c` (undo stack), `sim.c` (zener/MOSFET/pot); `docs/` + `upd_docs.py`; suite tes `selection` & `undo`. |
| **Ide Pengembangan** | Hapus yang sudah selesai (export PNG canvas, mini-map, toolbar scrollable, MOSFET); tambah model MOSFET linier. |
| *(baris lama — dipertahankan)* | |
| **Intro** | Menyebut probe osiloskop & pengukuran tegangan/arus. |
| **§ Osiloskop** | Cara pakai probe, anatomi trace, formula superposisi, batasan. |
| **Build** | Cara resmi sekarang `Makefile` (`make`, `make rebuild`, `make tests`, `make test`); `SRC_PROD` memakai `$(wildcard src/komponen/*.c)` sehingga komponen baru otomatis ikut. Skrip lama (`build.bat`/`build.sh`/`run_tests.sh`) hanya referensi. **Warning toolbar overflow** ditambahkan. |
| **Cara Pakai** | Bagian **2d/2e/2f/2g/2h**: waveform, voltmeter, ammeter, probe osiloskop, induktor. Langkah zoom/pan di §1. |
| **Troubleshooting** | Hapus "Save lalu Open menghilangkan LDR/Buzzer" (sudah diperbaiki); tambah ammeter/probe/induktor/zoom/toolbar-overflow. |
