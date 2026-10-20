## `docs/01-fitur.md`
# Fitur

[← Kembali ke readme](../readme.md)

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
| **Dioda Zener** | Pin A–K, arah sama dengan dioda biasa. **Forward**: menghantar seperti dioda. **Reverse**: breakdown bila $V_K - V_A > V_Z$ (default 5,1 V, edit via *Edit Nilai*, satuan `Vz`). Status forward/breakdown tampil saat RUN. |
| **MOSFET N-Channel** | 3 pin (gate, source, drain), model biner seperti transistor: **ON** (D–S di-union 0 Ω) bila gate tersambung ke net (+) dan source ke net (−). Nilai = $V_{th}$ (default 2 V). Saat RUN simbol hijau + label ON/OFF. |
| **Potensiometer** | 3 pin: A (kiri), B (kanan), W/wiper (atas-tengah). Dimodelkan sebagai **dua resistor seri**: $R_A = R \cdot \frac{\text{pos}}{100}$, $R_B = R \cdot \frac{100-\text{pos}}{100}$, clamp minimum 1 mΩ. Nilai = $R$ total (default **10 kΩ**); posisi wiper 0–100 % (default 50 %) via klik-kanan → *Posisi Wiper* (preset 0/10/25/50/75/90/100 %). |
| **Analisis nodal (MNA-lite)** | Mesin simulasi menyelesaikan **KCL** ($G \cdot V = I$) dengan Gauss elimination + partial pivoting. LED dihitung dengan $V_{th}$ & $R_{th}$ lalu $I = (V_{th} - V_f) / R_{th}$. Akurat untuk seri/paralel/pembagi tegangan (~1 % pada kasus tipikal). |
| **Voltmeter** | Ukur beda potensial dua titik, dipasang **paralel**, ideal (impedansi tak hingga → tidak mengubah rangkaian). Angka bacaan tampil di bawah simbol saat RUN. |
| **Ammeter** | Ukur arus lewat jalur, dipasang **seri**, shunt internal 0.1 Ω. Angka bacaan `A` tampil di bawah simbol saat RUN. |
| **Probe Osiloskop** | Lihat **bentuk gelombang AC terhadap waktu**. Maksimal 4 kanal, warna merah/hijau/biru/oranye. Panel terpisah via **View → Oscilloscope (F12)**; refresh otomatis 5 Hz. Lihat [docs/05-osiloskop.md](05-osiloskop.md). |
| **Induktor** | Simbol 4 lengkung. **DC**: short ideal (union seperti saklar-ON). **AC**: reaktansi $X_L = 2\pi f L$. Default 100 µH. |
| **Bentuk gelombang AC** | Klik-kanan Sumber AC → *Bentuk Gelombang* → Sinus / Kotak / Segitiga. Sumber AC default 220 VAC RMS @ 50 Hz; peak $= \sqrt{2} \cdot V_{rms}$. |
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
|   [Rangkaian BENAR -- LED MENYALA ]                        |
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
