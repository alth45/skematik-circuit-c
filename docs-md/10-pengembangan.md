## `docs/10-pengembangan.md`


# Ide Pengembangan

[← Kembali ke readme](../readme.md)

- **Transient solver** ($\frac{dI}{dt}$, $\frac{dV}{dt}$) — induktor & kapasitor
  akan punya bentuk gelombang RC/RL yang eksak, bukan hanya steady-state.
- **Phase offset per-sumber AC** (mis. 0°, 90°, 180°) — memungkinkan simulasi
  rangkaian 3-fasa sederhana.
- **Frekuensi per-sumber AC** (bukan hanya 50 Hz global).
- **Skala timebase & voltase oscilloscope manual** (dropdown 1× / 10× / auto).
- **Freeze / capture** trace oscilloscope (pause tombol di panel).
- **Export PNG dari oscilloscope** (canvas sudah bisa via `Ctrl+E`).
- **Paste dari clipboard** (Ctrl+C / Ctrl+V) untuk blok terpilih.
- **Model transistor $\beta$/$I_b$/$I_c$** dengan $V_{be} \approx 0.7\,\text{V}$.
- **Dioda Shockley-lite** ($V_f$ menyesuaikan arus) untuk presisi > 1 %.
- **Model MOSFET linier/ohmic** ($R_{ds(on)}$, region aktif) — saat ini masih biner.
- **LED $V_f$ dinamis** ($V_f = V_{f0} + R_s \cdot I$).
- **Net label** / port untuk multi-sheet.
- **Autosave** + folder `examples/*.json`.
- **Komponen tambahan**: push-button, saklar SPDT, PNP, IC op-amp sederhana, IC 555.

---
