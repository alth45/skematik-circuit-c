## `docs/09-troubleshooting.md`


# Troubleshooting

[← Kembali ke readme](../readme.md)

| Error | Solusi |
|---|---|
| `warning: 'V_UNKNOWN' redefined` | Bentrok dengan `oleauto.h`. Ganti nama jadi `SIM_V_UNKNOWN` di `sim.c`. |
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
| Induktor seperti short di AC | $X_L = 2\pi f L$; untuk $L = 100\,\mu\text{H}$ di 50 Hz memang $\approx 0.03\,\Omega$ (mendekati short). Perbesar nilai (`1`, `10m`, dst). |
| Rotasi simbol "pecah" (simbol keluar dari kotak seleksi / pin lepas) | Fitur zoom + rotasi harus dikomposisi di `with_rotation`. Pastikan `draw.c` memakai versi terbaru (yang menyimpan transform parent via `GetWorldTransform` dan memanggil `SetWorldTransform(&prev)` setelah `RestoreDC`). |
| Tombol Save/Open/Run di toolbar tidak terlihat | Toolbar overflow. Gunakan `Ctrl+S`, `Ctrl+O`, `F5`, atau maximize jendela. |
| Save lalu Open menghilangkan LDR/Buzzer | Sudah diperbaiki. Kalau masih hilang, cek versi `fileio.c` — pastikan `comp_type_name`/`comp_type_from_name` sudah mencakup `"ldr"` & `"buzzer"`. |
| Gerbang logika selalu LOW | Level HIGH ditentukan dari keterhubungan ke net **(+)**: sambungkan input ke pin baterai (+) (atau output gerbang HIGH lain). Input mengambang = LOW. |
| Output gerbang tidak menggerakkan beban | Output yang HIGH di-union ke net (+); pastikan beban (LED) punya jalur balik ke (−)/ground. |
| Gerbang multi-tingkat salah | Rantai gerbang diselesaikan **iteratif**; pastikan wiring antar tahap benar. Loop stabilisasi berhenti saat semua level tetap. |
| Buzzer tidak berbunyi | Saat RUN, buzzer butuh beda tegangan pin > 1 V. Cek kabel (+)/(−) & saklar. Frekuensi 20 Hz–20 kHz (di luar itu di-clamp). |
| Buzzer bersuara walau tidak diminta | Buzzer aktif otomatis bila tercatu. Hapus/nonaktifkan jalurnya, atau Stop simulasi. |
| `undefined reference to buzzer_audio_update` | `buzzer_audio.c` belum masuk `SRC_PROD` di `Makefile`; linkage butuh `-lwinmm`. |

---
