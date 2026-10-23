# Aturan Simulasi (Analisis Nodal)

[← Kembali ke readme](../readme.md)

Mesin `sim.c` memakai **union-find** untuk kabel + penghantar ideal 0 Ω,
lalu **analisis nodal (MNA-lite)** untuk menyelesaikan tegangan/arus.

## Tahap 1 — Topologi (union-find)

1. Kabel menyatukan pin-pin menjadi satu net (root).
2. **Penghantar IDEAL 0 Ω** yang di-union hanya: saklar-ON, **induktor saat
   mode DC**, dioda/Zener forward, transistor-ON / **MOSFET-ON**.
   - Resistor, LDR, buzzer, ammeter **tidak** di-union — tetap edge berbobot $R$.
   - Kapasitor / induktor dalam mode AC juga tidak di-union — jadi edge reaktif.
   - **Zener reverse**: bukan union — dimodelkan sebagai sumber tegangan $V_Z$
     ($K = +$), status breakdown dicari iteratif (maks. 20 iterasi) sampai stabil.
3. Net (+): semua pin0 baterai + pin0 (Live) sumber AC. Net (−): pin1 sumber +
   pin ground.
4. **Gerbang logika (iteratif)**: level tiap gerbang dihitung dari topologi —
   **HIGH** bila node input terhubung ke net **+**, selain itu **LOW**. Output
   gerbang di-union ke net (+) saat HIGH / ground·(−) saat LOW. Karena gerbang
   berikutnya bergantung pada output gerbang sebelumnya, blok ini dijalankan
   **berulang** (loop stabilisasi) sampai tak ada level yang berubah — sehingga
   rangkaian **multi-tingkat** (mis. AND → NOT → LED) tetap benar.

## Tahap 2 — Analisis nodal

5. Matriks konduktansi $G$ dibangun dari semua edge resistif:
   - Resistor → $R = c_v$
   - LDR → $R_{\text{gelap}} \times 1\%$ saat Terang, atau $R_{\text{gelap}}$ saat Gelap
   - Buzzer → $R = 100\,\Omega$ (tipikal)
   - Ammeter → $R = 0.1\,\Omega$
   - Kapasitor (AC) → $X_C = \dfrac{1}{2\pi f C}$
   - Induktor (AC) → $X_L = 2\pi f L$
   - **Potensiometer** → dua edge seri:
     $$
     R_A = R \cdot \frac{\text{pos}}{100}, \qquad
     R_B = R \cdot \frac{100-\text{pos}}{100}
     $$
     dengan clamp minimum 1 mΩ.
6. Sumber tegangan → node dengan $V$ **tetap** (baris constraint identitas).
7. Solve $G \cdot V = I$ dengan **Gauss + partial pivoting**. Regularisasi
   $10^{-12}$ cegah singular.

## Tahap 3 — Perhitungan LED

8. Per LED: hitung $V_{th}$ (V anoda − V katoda dari solve global) dan $R_{th}$
   (semua sumber di-short, injeksi 1 A, solve ulang).
   $$
   I = \frac{V_{th} - V_f}{R_{th}}
   $$
9. Klasifikasi:
   - $R_{th} \approx 0$ & $V_{th} > V_f$ → **MELEDAK**.
   - $I > 100\,\text{mA}$ → **MELEDAK**.
   - $20\,\text{mA} < I \le 100\,\text{mA}$ → **overcurrent** (peringatan).
   - $I \le 20\,\text{mA}$ → **AMAN**.

## Tahap 4 — Periferal

10. Korsleting: (+) & (−) satu root (termasuk lewat dioda forward).
11. **Buzzer** ON bila $\left|V(\text{pin0}) - V(\text{pin1})\right| > 1\,\text{V}$,
    atau seri/paralel dengan LED yang menyala.
12. **Voltmeter** ideal → baca $V(\text{pin0}) - V(\text{pin1})$. Kedua pin tidak
    pernah di-union.
13. **Ammeter** → baca
    $$
    I = \frac{\left|V(\text{pin0}) - V(\text{pin1})\right|}{0.1\,\Omega}
    $$
14. **Probe OSC** → baca $V_{dc} + \sum_k g_k \cdot V_{ac,k}(t)$ via superposisi
    (dihitung lazy setelah `sim_recompute`).
15. Kabel hot: net yang memuat (+) sumber → merah tebal.
16. **Buzzer audio**: bila ada buzzer aktif (beda pin > 1 V), nada sinus pada
    frekuensi `v` diputar via `PlaySound` (WAV in-memory); restart otomatis bila
    frekuensi berubah, stop saat tidak ada buzzer aktif / RUN berhenti.

## Bentuk gelombang AC

- Peak:
  $$
  V_{pk} = \sqrt{2} \cdot V_{rms}
  $$
- $\text{WAVE\_SINE} \;\rightarrow\; V_{pk} \cdot \sin(2\pi f t)$
- $\text{WAVE\_SQUARE} \;\rightarrow\; +V_{pk}$ di paruh periode positif,
  $-V_{pk}$ di negatif
- $\text{WAVE\_TRIANGLE} \;\rightarrow\;$ naik linear
  $0 \to V_{pk} \to 0 \to -V_{pk}$ dalam satu periode

## Akurasi

- **Seri, paralel, campuran, pembagi tegangan**: ~1 % (batas praktis `%.4g`).
- **Model LED**: $V_f$ konstan.
- **Transistor**: biner (ON/OFF berdasarkan bias basis-emitor), belum model $\beta$.
- **MOSFET**: biner seperti transistor — ON bila gate di net (+) & source di
  net (−), D–S short 0 Ω; belum model linier/ohmic.
- **Dioda**: ideal satu arah, $V_f = c_v$ (default 0.7 V).
- **Zener**: forward ideal; reverse breakdown dikunci di tepat $V_Z$
  (default 5.1 V), tanpa resistansi dinamis.
- **Potensiometer**: model resistif linear murni (dua edge seri, tanpa clamp
  non-linear); akurat sesuai pembagian $\text{pos}\%$.
- **Induktor**: DC = short ideal, AC = $X_L$ linier.
- **Probe OSC**: eksak via superposisi (linear).
- **Gerbang logika**: ideal biner, tanpa delay propagasi / fan-out maksimum.
  HIGH ditentukan semata dari keterhubungan ke net (+) (bukan ambang tegangan).
- **Buzzer audio**: satu kanal (buzzer aktif pertama), nada sinus; frekuensi
  di-clamp ke rentang 20 Hz–20 kHz.

## Pesan hasil

- `Belum ada sumber (baterai/AC) — LED tidak bisa menyala.`
- `KORSLETING! (+) tersambung langsung ke (-).`
- `BAHAYA! LED MELEDAK — arus ... melebihi batas 20 mA. Tambah/perbesar resistor seri!`
- `PERINGATAN! LED rusak pelan — arus ... > 20 mA (max). Perbesar resistor seri.`
- `Rangkaian BENAR — LED MENYALA. I = ..., R_loop = ..., Vbat = ... V.`
- `Rangkaian AC BENAR — LED MENYALA (pendekatan RMS). I = ..., R_loop = ..., VAC = ... V.`
- `Rangkaian belum lengkap — LED mati. Tutup saklar & sambungkan kabel.`

---