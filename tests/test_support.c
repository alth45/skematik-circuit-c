/* test_support.c — Penunjang suite uji MURNI (HANYA untuk run_tests.sh)
   =======================================================================
   Pemisahan ketat tes vs produksi:
     - Berkas ini TIDAK PERNAH masuk daftar sumber build.bat / executable
       produksi. Build produksi pun menolak mode tes lewat #error di bawah.
     - Semua simbol di sini (test_reset_model, stub set_status/g_status)
       adalah infrastruktur uji; tidak ada satu pun yang bocor ke binary
       aplikasi.
   Catatan: stub LDR/Buzzer lama (unit_stubs.c + seam suntik di schematic.h)
   sudah DIHAPUS — implementasi aslinya kini tinggal di src/sim.c dan unit
   test menguji kontrak tersebut langsung terhadap kode produksi. */

#include "schematic.h"

#if !defined(SCHEMATIC_TEST_STUBS)
#error "test_support.c adalah berkas TES: wajib dikompilasi dengan -DSCHEMATIC_TEST_STUBS=1 (lihat pemisah mode di schematic.h)."
#endif

/* ===================== Helper reset state global model =====================
   Semua test memakai model.* yang bertumpu pada variabel global; reset
   dipanggil di awal tiap test agar antar-test saling bebas. Deklarasi
   fungsi ada di schematic.h (blok SCHEMATIC_TEST_STUBS). */
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
    undo_reset();
}

/* ---- Stub set_status/g_status (realisasi di src/fileio.c tidak ditautkan
       ke binary uji agar suite bebas dari dialog Win32) ---- */
#include <stdarg.h>

wchar_t g_status[256] = L"";

void set_status(const wchar_t *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf(g_status, 256, fmt, ap);
    g_status[255] = L'\0';
    va_end(ap);
}
