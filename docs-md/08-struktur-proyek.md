## `docs/08-struktur-proyek.md`


# Struktur Proyek

[← Kembali ke readme](../readme.md)

```
schematic-c/
  include/
    schematic.h        konstanta, struct, API semua modul (enum tipe komponen & tool)
  src/
    main.c             WndProc, toolbar_click, WM_PAINT (zoom/pan, banner, status),
                       render_canvas_scene, accelerator (Ctrl+N/O/S/E, F5/F9/F12), wWinMain
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
                       keyboard (0-9, J, I, P, L, M, U, R, Z, F, T, F5, F12,
                                 Ctrl+S/O/A/D/E/Z/Y, Ctrl +/-/0, Del, Esc)
                       + menu konteks + waveform picker + preset wiper potensiometer
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
  tests/                 suite uji (target `make tests` / `make test`):
                         model, sim, selection, undo
  docs/                  dokumentasi HTML API (index per modul + halaman per fungsi;
                         dibuat/machine-update via upd_docs.py)
  Makefile               build resmi: app + tests + clean + rebuild
  build.bat / build.sh / run_tests.sh   skrip lama, hanya referensi
  readme.md              file ini
```

## Alur data

`events/toolbar` → `model` → `sim_recompute`
(union-find → analisis nodal → per-LED → periferal) →
`draw/main` baca:

- `sim_wire_hot`
- `sim_comp_lit`
- `sim_voltmeter_reading`
- `sim_ammeter_reading`
- `sim_probe_voltage`
- `sim_message`

`oscilloscope.c` meng-query `sim_probe_voltage` tiap 200 ms untuk trace.

---
