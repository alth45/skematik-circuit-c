## `docs/11-changelog.md`

# Ringkasan Perubahan

[← Kembali ke readme](../readme.md)

| Bagian | Yang berubah |
|---|---|
| **Fitur table** | Baris **Undo/Redo** (Ctrl+Z/Y, depth 64), **Dioda Zener**, **MOSFET N-Channel**, **Potensiometer**, **Export PNG**, **Minimap** ditambahkan. Toolbar kini **23 tombol palet**; daftar komponen & nilai (Zener $V_Z$, MOSFET $V_{th}$, Pot Ω + wiper) diperbarui. |
| **Tampilan (ASCII)** | Baris toolbar 23 tombol (termasuk ZEN/FET/POT); status bar `zoom XX%`. |
| **Shortcut** | `Z` (Zener), `F` (MOSFET), `T` (Potensiometer), `Ctrl+Z/Y` (undo/redo), `Ctrl+E` (export PNG), `I`, `P`, `J`, `F12`, `Ctrl +/-/0`, mouse wheel, mouse tengah. |
| **Format JSON** | `type` baru: `zener \| mosfet \| potentiometer` (+ alias `Z/ZD/zenerdiode`, `nmos/FET`, `pot/trimmer/rheostat`). `value` = $V_Z$ (V) / $V_{th}$ (V) / $R$ total (Ω); wiper pot di field `on`. |
| **Aturan Simulasi** | Tahap 1 menyebut dioda/**Zener forward** + **MOSFET-ON**; Zener reverse = sumber $V_Z$ iteratif. Tahap 2: edge **potensiometer** (A–W / W–B). Akurasi mencantumkan batas model Zener, MOSFET biner & POT linear. |
| **Struktur Proyek** | `png_writer.c`, `ui/export_png.c`, `ui/minimap.c`, `komponen_zener.c`, `komponen_mosfet.c`, `komponen_pot.c`; `model.c` (undo stack), `sim.c` (zener/MOSFET/pot); `docs/` + `upd_docs.py`; suite tes `selection` & `undo`. |
| **Ide Pengembangan** | Hapus yang sudah selesai (export PNG canvas, mini-map, toolbar scrollable, MOSFET); tambah model MOSFET linier. |
| **Intro** | Menyebut probe osiloskop & pengukuran tegangan/arus. |
| **§ Osiloskop** | Cara pakai probe, anatomi trace, formula superposisi, batasan. |
| **Build** | Cara resmi sekarang `Makefile` (`make`, `make rebuild`, `make tests`, `make test`); `SRC_PROD` memakai `$(wildcard src/komponen/*.c)` sehingga komponen baru otomatis ikut. Skrip lama (`build.bat`/`build.sh`/`run_tests.sh`) hanya referensi. **Warning toolbar overflow** ditambahkan. |
| **Cara Pakai** | Bagian **2d/2e/2f/2g/2h**: waveform, voltmeter, ammeter, probe osiloskop, induktor. Langkah zoom/pan di §1. |
| **Troubleshooting** | Hapus "Save lalu Open menghilangkan LDR/Buzzer" (sudah diperbaiki); tambah ammeter/probe/induktor/zoom/toolbar-overflow. |



## 📐 Ringkasan Konversi Formula ke KaTeX

| Sebelum (plain) | Sesudah (KaTeX) |
|---|---|
| `V_K − V_A > Vz` | $V_K - V_A > V_Z$ |
| `XL = 2πfL` | $X_L = 2\pi f L$ |
| `Xc = 1/(2πfC)` | $X_C = \dfrac{1}{2\pi f C}$ |
| `I = (Vth − Vf) / Rth` | $I = \dfrac{V_{th} - V_f}{R_{th}}$ |
| `R_A = R·pos%` | $R_A = R \cdot \frac{\text{pos}}{100}$ |
| `G·V = I` | $G \cdot V = I$ |
| `I = ΔV / 0.1` | $I = \dfrac{\Delta V}{0.1\,\Omega}$ |
| `Vpk = √2 · Vrms` | $V_{pk} = \sqrt{2} \cdot V_{rms}$ |
| `V_probe(t) = V_dc + Σ gain_k·V_ac_k(t)` | $V_{\text{probe}}(t) = V_{dc} + \sum_{k} g_k \cdot V_{ac,k}(t)$ |
| `Vpk·sin(2πft)` | $V_{pk} \cdot \sin(2\pi f t)$ |
| `\|V(pin0) − V(pin1)\| > 1 V` | $\left\|V(\text{pin0}) - V(\text{pin1})\right\| > 1\,\text{V}$ |
| `20 mA < I ≤ 100 mA` | $20\,\text{mA} < I \le 100\,\text{mA}$ |
| `Vbe ≈ 0.7 V` | $V_{be} \approx 0.7\,\text{V}$ |
| `Vf0 + Rs·I` | $V_f = V_{f0} + R_s \cdot I$ |
| `Rds(on)` | $R_{ds(on)}$ |

