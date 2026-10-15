/* unit_stubs.c — Stub terkontrol untuk unit test (mode SCHEMATIC_TEST_STUBS)
   ========================================================================
   PENTING: berkas ini HANYA untuk suite uji. Build aplikasi produksi
   (build.bat / run_tests.sh mode verifikasi) tidak pernah menautkannya.

   Latar belakang: dua fungsi yang DIPANGGIL kode produksi —
     - sim_buzzer_on(c)        (dipakai komponen_buzzer.c, events.c)
     - sim_ldr_effective_r(c)  (dipakai events.c)
   belum punya implementasi di src/ (sim.c belum menangani COMP_LDR /
   COMP_BUZZER). Suite uji harus bisa berjalan SEKARANG terhadap kontrak
   header, jadi kedua fungsi direalisasikan sebagai "stub kuning":
     - default mengembalikan nilai netral (0 = diam, -1 = bukan LDR),
     - dapat dioverride per-test lewat function pointer g_*_impl,
       sehingga perilaku tiap fungsi bisa disuntik sesuai skenario.

   Ketika implementasi asli masuk ke sim.c, cukup hapus blok test-seam di
   schematic.h dan berkas ini; test yang sudah ada tetap berlaku karena
   mereka menguji KONTRAK (nilai yang diharapkan pemanggil), bukan stubnya. */

#include "schematic.h"

#if !defined(SCHEMATIC_TEST_STUBS)
#error "unit_stubs.c harus dikompilasi dengan -DSCHEMATIC_TEST_STUBS=1"
#endif

/* ---- Stub default: netral, tidak memicu efek visual/logika apa pun ---- */
static int default_stub_buzzer_on(const Component *c) {
    (void)c;
    return 0; /* diam */
}

static double default_stub_ldr_effective_r(const Component *c) {
    (void)c;
    return -1.0; /* "-1 bukan LDR" sesuai komentar kontrak di header */
}

sim_buzzer_on_fn       g_sim_buzzer_on_impl       = default_stub_buzzer_on;
sim_ldr_effective_r_fn g_sim_ldr_effective_r_impl = default_stub_ldr_effective_r;

/* ===================== Helper reset state global model =====================
   Semua test memakai model.* yang bertumpu pada variabel global; reset
   dipanggil di awal tiap test agar antar-test saling bebas. */
void test_reset_model(void) {
    comp_count = 0;
    wire_count = 0;
    next_id = 1;
    selected_id = -1;
    dragging_id = -1;
    drag_dx = drag_dy = 0;
    current_tool = TOOL_SELECT;
    wiring_active = 0;
    wiring_from.comp_id = -1;
    wiring_from.pin_idx = -1;
    wiring_mx = wiring_my = 0;
    sim_running = 0;
    g_show_grid = 1;
    g_show_status = 1;
    g_sim_buzzer_on_impl = default_stub_buzzer_on;
    g_sim_ldr_effective_r_impl = default_stub_ldr_effective_r;
}
