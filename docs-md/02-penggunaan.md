## `docs/02-penggunaan.md`


# Cara Pakai

[← Kembali ke readme](../readme.md)

## 1. Merakit (mode Edit)

1. Klik ikon komponen (mis. BAT), lalu klik canvas untuk menaruh.
2. Klik ikon Wire, tekan-tahan titik merah satu pin, tarik ke pin lain, lepas.
   Trik cepat: di mode Select langsung tarik pin → pin.
3. Geser: mode Select → tekan badan simbol → tahan → geser → lepas.
4. Putar: klik simbol sampai kotak putus-putus muncul → tekan `R`.
5. Atur nilai: double-klik simbol (atau klik-kanan → *Edit Nilai*…).
6. Klik-kanan simbol: menu konteks (Edit Nilai, Toggle saklar, Putar, Duplikat,
   **Bentuk Gelombang** untuk AC, **Buka Osiloskop** untuk probe, Info, Hapus).
7. Zoom / pan: **wheel** = zoom di kursor; **Ctrl++/−** = zoom di tengah;
   **Ctrl+0** = zoom fit; **mouse tengah + drag** = pan.

## 2. Rangkaian LED menyala

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
- **Transistor NPN**: pin0 = Kolektor (kanan-atas), pin1 = Emitor (kanan-bawah),
  pin2 = Basis (kiri-tengah). **Simbol NPN standar**: base di kiri, C & E di kanan,
  panah keluar di emitor.

## 2b. Proteksi polaritas dengan dioda

```
BAT (+) -- kabel -- D (A→K) -- kabel -- LED pin0
BAT (-) -- kabel -------------------------- LED pin1
```

Saat RUN, dioda yang forward ditandai panah arah arus hijau. Jika reverse-bias,
loop terbuka.

## 2c. Sumber AC + kapasitor

Ganti BAT dengan ikon **AC** (shortcut `0`). Sumber AC: pin0 = Live, pin1 = Netral,
default 220 VAC RMS @ 50 Hz.

```
AC Live   -- kabel -- SW -- kabel -- R -- kabel -- LED anoda
AC Netral -- kabel ----------------------------- LED katoda
```

Jika hanya sumber AC (tanpa baterai), simulator masuk **mode AC**:
- Kapasitor → reaktansi $X_C = \dfrac{1}{2\pi f C}$ (bukan short).
- **Induktor → reaktansi $X_L = 2\pi f L$** (bukan short).

## 2d. Bentuk gelombang AC (sinus / kotak / segitiga)

Klik-kanan Sumber AC → **Bentuk Gelombang** → pilih Sinus / Kotak / Segitiga.
Bentuk gelombang ini hanya memengaruhi **probe osiloskop** (simulasi DC & LED tetap
memakai pendekatan RMS).

## 2e. Voltmeter

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
- Bacaan $= V(\text{pin+}) - V(\text{pin-})$. Kutub terbalik → negatif +
  `(kutub terbalik)` di info.
- Salah satu ujung mengambang → `-`.

## 2f. Ammeter

Ammeter dipasang **seri** — potong jalur dan masukkan ammeter di antaranya:

```
BAT (+) -- (AM) pin0 (+) -- R -- LED -- BAT (−)
              └─ pin1 (−) keluar jalur
```

- Klik **AM** (`I`), taruh di antara dua komponen seri.
- Tekan **F5** → bacaan (mis. `31.8 mA`, hijau) tampil di bawah simbol.
- Shunt internal $0.1\,\Omega$ — drop kecil ($< 5\,\text{mV}$ pada 30 mA),
  tidak mengubah arus secara signifikan tapi cukup untuk menghitung
  $I = \dfrac{\Delta V}{0.1\,\Omega}$ dengan stabil.

## 2g. Probe osiloskop

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

## 2h. Induktor

- Klik **L** (`J`), taruh di jalur.
- **DC (baterai)**: induktor short ideal. Arus mengalir seperti tanpa komponen.
- **AC (sumber AC)**: reaktansi $X_L = 2\pi \cdot 50 \cdot L$ Ω.
  - $L = 100\,\mu\text{H} \Rightarrow X_L \approx 0.03\,\Omega$ (hampir short).
  - $L = 100\,\text{mH} \Rightarrow X_L \approx 31\,\Omega$.
  - $L = 1\,\text{H} \Rightarrow X_L \approx 314\,\Omega$ (LED meredup/hampir mati).
- Double-klik untuk edit nilai (`100u`, `10m`, `1`, dst).

## 2i. Gerbang logika (AND / OR / NOT / NAND / NOR / XOR)

- Klik ikon gerbang di toolbar (atau shortcut **A** AND, **O** OR, **N** NOT,
  **Y** NAND, **X** XOR; **NOR** lewat toolbar).
- Pin gerbang 2-input: **A** (kiri-atas), **B** (kiri-bawah), **OUT** (kanan).
  Gerbang **NOT**: **IN** (kiri) dan **OUT** (kanan).
- Level logika ditentukan dari topologi rangkaian:
  - **HIGH (1)** bila node (input/output) terhubung ke net **(+)**, mis. ke pin
    baterai (+) atau ke output gerbang lain yang HIGH.
  - **LOW (0)** bila tidak.
- Output gerbang di-union ke net (+) saat HIGH (atau ke ground·(−) saat LOW),
  sehingga rangkaian **multi-tingkat** diselesaikan secara **iteratif** sampai
  stabil.
- Saat **RUN**, gerbang yang outputnya HIGH digambar **hijau**; statuspun tampil
  di **Info** (`Output RUN: HIGH (1)` / `LOW (0)`).

```
BAT (+) --+-- [A]                       
          |         \
          +-- [B]  [AND] -- OUT -- LED anoda
          |         /
BAT (-) --+---------+----------- LED katoda   (mis. OUT drive LED)
```

## 2j. Buzzer berbunyi

- Klik **BZR** (`U`), taruh di rangkaian, beri catu (beda tegangan pin > 1 V).
- Saat **RUN**, buzzer aktif **berbunyi** pada frekuensi `v` (default 1000 Hz)
  sebagai nada **sinus** (loop mulus, tanpa klik).
- Double-klik buzzer untuk mengubah frekuensi (mis. `440`, `2k`, `8k`).
- Hanya **buzzer aktif pertama** yang diputar; restart otomatis bila frekuensi
  berubah. Nada berhenti saat **Stop**.

## 3. Run simulasi

1. Klik tombol hijau **Run** (atau `F5`).
2. Klik **saklar** di canvas untuk ON/OFF. LED langsung update.
3. Baca banner tengah + status bar bawah.
4. Klik **Stop** (atau `F5`) untuk kembali edit.

Saat RUN dikunci: tambah/geser/putar/hapus, tarik kabel baru, ganti tool,
dan zoom/pan juga dikunci.

---
