## `docs/05-osiloskop.md`


# Osiloskop (Probe + Window)

[← Kembali ke readme](../readme.md)

## Cara pakai cepat

1. Klik ikon **OSC** di toolbar (atau tekan `P`), klik canvas untuk menaruh probe.
2. Sambungkan **pin kiri** probe (sinyal / tip) ke titik yang ingin diukur, dan
   **pin kanan** (klip referensi) ke titik referensi (biasanya ground / netral).
3. Tekan **F12** atau **View → Oscilloscope** → jendela oscilloscope terbuka.
4. Trace muncul & auto-refresh ~5 Hz. Ubah rangkaian di canvas (saklar, waveform,
   nilai komponen) — trace ikut update.

## Anatomi trace

- Timebase tetap **80 ms** (4 periode @ 50 Hz) → 8 ms per division.
- Skala vertikal **otomatis** — dipilih sebagai pangkat dua terkecil yang masih
  memuat semua trace.
- Nilai yang ditampilkan:

$$
V_{\text{probe}}(t) = V_{dc} + \sum_{k} g_k \cdot V_{ac,k}(t)
$$

  - $V_{dc}$ = respon saat semua sumber AC dimatikan (hanya sumber DC).
  - $g_k$ = $V_{\text{probe}}$ saat sumber AC ke-$k$ diset 1 V (unit), sisanya 0.
  - $V_{ac,k}(t)$ = gelombang instan sumber AC (sinus / kotak / segitiga).

- Cara ini murah dan **eksak** untuk rangkaian linear (superposisi), tanpa
  transient solver.

## Batasan

- Frekuensi tetap **50 Hz** (sama seperti model AC).
- Hanya resistor & sumber AC yang dimodelkan dalam respon frekuensi.
  Kapasitor/induktor **ikut** karena diperlakukan sebagai edge reaktif
  ($X_C$, $X_L$).
- Semua sumber AC diasumsikan **sefasa**. Phase offset per-source belum ada.

---
