/* stubs_production.c — Stub sementara untuk fungsi yang belum diimplementasikan
   ===========================================================================
   DIPAKAI HANYA oleh build aplikasi produksi (lihat build.bat).
   JANGAN menautkan berkas ini ke suite unit test (tests/unit_stubs.c adalah
   versi test-nya; keduanya tidak boleh ada dalam satu binary).

   Konteks: commit "Add LDR and Buzzer components" menambahkan pemanggilan
   terhadap sim_buzzer_on() dan sim_ldr_effective_r() (di src/ui/events.c dan
   src/komponen/komponen_buzzer.c) beserta deklarasi di include/schematic.h,
   TETAPI implementasinya belum ditambahkan ke src/sim.c. Akibatnya build
   produksi gagal saat linking (undefined reference).

   TODO(feature-ldr-buzzer-sim): pindahkan logika sebenarnya ke sim.c:
     - sim_ldr_effective_r(): tipe != COMP_LDR -> -1;
       gelap (on==0) -> c->v; terang (on==1) -> c->v * LDR_LIGHT_RATIO.
     - sim_buzzer_on(): 1 bila kedua pin buzzer terhubung ke kutub (+)/(-)
       lewat jalur menghantar saat mode RUN.
   Setelah itu hapus berkas ini dan blok "Test seam" pada schematic.h. */

#include "schematic.h"

#ifdef SCHEMATIC_TEST_STUBS
#error "stubs_production.c tidak boleh dikompilasi untuk suite uji (pakai tests/unit_stubs.c)"
#endif

int sim_buzzer_on(const Component *c) {
    /* TODO(feature-ldr-buzzer-sim): stub kuning — selalu "diam". */
    (void)c;
    return 0;
}

double sim_ldr_effective_r(const Component *c) {
    /* TODO(feature-ldr-buzzer-sim): stub kuning — kembalikan -1
       ("bukan LDR") sesuai kontrak di schematic.h agar pemanggil UI
       menampilkan nilai netral alih-alih membaca memori tak terinisialisasi. */
    (void)c;
    return -1.0;
}
