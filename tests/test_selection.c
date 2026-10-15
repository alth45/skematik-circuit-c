/* test_selection.c — Unit test fitur MULTI-SELECT / BLOK RANGKAIAN
   ===================================================================
   Mencakup API seleksi & operasi grup di src/model.c yang dipakai fitur
   "blok rangkaian lalu drag-and-drop":
     - add_to_selection / toggle_selection / clear_selection
     - model_translate_selection (rigid group move)
     - model_delete_selection    (multi-delete + kabel ikut terhapus)
     - model_duplicate_selection (Ctrl+D, kabel internal ikut tersalin)
     - state group_dragging / group_drag_items / group_drag_n */

#include "schematic.h"
#include "tinytest.h"

void test_reset_model(void); /* dari tests/unit_stubs.c */

/* Helper: buat komponen resistor di grid dan kembalikan id-nya. */
static int add_res_at(int x, int y) {
    model_add_component(COMP_RESISTOR, x, y);
    return selected_id; /* model_add_component menyeleksi yang baru dibuat */
}

/* Helper: sambungkan pin pa dari komponen id_a ke pin pb dari id_b. */
static void wire_ids(int id_a, int pa, int id_b, int pb) {
    PinRef a = { id_a, pa }, b = { id_b, pb };
    model_add_wire(a, b);
}

/* ===================== add_to_selection ===================== */

TEST(selection, add_menumpuk_id_unik_dalam_urutan_klik) {
    test_reset_model();
    int r1 = add_res_at(100, 100);
    int r2 = add_res_at(200, 100);
    int r3 = add_res_at(300, 100);

    clear_selection();
    ASSERT_INT_EQ(1, add_to_selection(r1));
    ASSERT_INT_EQ(1, add_to_selection(r2));
    ASSERT_INT_EQ(1, add_to_selection(r3));
    ASSERT_INT_EQ(3, sel_count);
    ASSERT_INT_EQ(r1, selection[0]);   /* urutan = urutan klik */
    ASSERT_INT_EQ(r2, selection[1]);
    ASSERT_INT_EQ(r3, selection[2]);
}

TEST(selection, add_menolak_duplikat_id) {
    test_reset_model();
    int r1 = add_res_at(100, 100);
    clear_selection();
    add_to_selection(r1);
    ASSERT_INT_EQ(0, add_to_selection(r1));  /* duplikat ditolak */
    ASSERT_INT_EQ(1, sel_count);
}

TEST(selection, add_menolak_id_tak_valid) {
    test_reset_model();
    clear_selection();
    ASSERT_INT_EQ(0, add_to_selection(-5));  /* id negatif */
    ASSERT_INT_EQ(0, add_to_selection(9999));/* tidak ada komponennya */
    ASSERT_INT_EQ(0, sel_count);
}

/* ===================== toggle_selection (Ctrl+klik) ===================== */

TEST(selection, toggle_memasukkan_lalu_mengeluarkan_id) {
    test_reset_model();
    int r1 = add_res_at(100, 100);
    int r2 = add_res_at(200, 100);
    clear_selection();

    toggle_selection(r1);
    toggle_selection(r2);
    ASSERT_INT_EQ(2, sel_count);

    toggle_selection(r1);                    /* keluar lagi */
    ASSERT_INT_EQ(1, sel_count);
    ASSERT_INT_EQ(r2, selection[0]);
}

/* ===================== clear_selection ===================== */

TEST(selection, clear_mengosongkan_sel_count_dan_selected_id) {
    test_reset_model();
    int r1 = add_res_at(100, 100);
    clear_selection();
    add_to_selection(r1);
    selected_id = r1;

    clear_selection();
    ASSERT_INT_EQ(0, sel_count);
    ASSERT_INT_EQ(-1, selected_id);
}

/* ===================== model_translate_selection ===================== */

TEST(selection, translate_gerakkan_semua_anggota_rigid) {
    test_reset_model();
    int r1 = add_res_at(100, 100);
    int r2 = add_res_at(140, 100);
    int r3 = add_res_at(180, 160);
    clear_selection();
    add_to_selection(r1); add_to_selection(r2); add_to_selection(r3);

    model_translate_selection(60, -40);

    Component *c1 = find_comp_by_id(r1);
    Component *c2 = find_comp_by_id(r2);
    Component *c3 = find_comp_by_id(r3);
    ASSERT_TRUE(c1 && c2 && c3);
    ASSERT_INT_EQ(160, c1->x); ASSERT_INT_EQ(60,  c1->y);
    ASSERT_INT_EQ(200, c2->x); ASSERT_INT_EQ(60,  c2->y);
    ASSERT_INT_EQ(240, c3->x); ASSERT_INT_EQ(120, c3->y);
    /* jarak antar anggota tak berubah (blok kaku) */
    ASSERT_INT_EQ(c2->x - c1->x, 40);
    ASSERT_INT_EQ(c3->x - c1->x, 80);
}

TEST(selection, translate_nol_dan_kosong_aman) {
    test_reset_model();
    int r1 = add_res_at(100, 100);
    clear_selection();
    model_translate_selection(50, 50);       /* seleksi kosong: no-op */
    Component *c = find_comp_by_id(r1);
    ASSERT_TRUE(c);
    ASSERT_INT_EQ(100, c->x); ASSERT_INT_EQ(100, c->y);

    add_to_selection(r1);
    model_translate_selection(0, 0);         /* delta nol: no-op */
    ASSERT_INT_EQ(100, find_comp_by_id(r1)->x);
}

/* ===================== model_delete_selection ===================== */

TEST(selection, delete_hapus_seluruh_blok_dan_kabel_ikutannya) {
    test_reset_model();
    int r1 = add_res_at(100, 100);
    int r2 = add_res_at(200, 100);
    int luar = add_res_at(400, 400);
    wire_ids(r1, 0, r2, 0);                  /* kabel internal blok */
    wire_ids(r2, 1, luar, 0);                /* kabel keluar blok */
    int wires_before = wire_count;
    ASSERT_INT_EQ(2, wires_before);

    clear_selection();
    add_to_selection(r1); add_to_selection(r2);

    int n = model_delete_selection();
    ASSERT_INT_EQ(2, n);                     /* 2 komponen terhapus */
    ASSERT_NULL(find_comp_by_id(r1));
    ASSERT_NULL(find_comp_by_id(r2));
    ASSERT_TRUE(find_comp_by_id(luar));      /* di luar blok tetap ada */
    /* kedua kabel menempel pada komponen yang dihapus -> ikut hilang */
    ASSERT_INT_EQ(0, wire_count);
    ASSERT_INT_EQ(0, sel_count);
    ASSERT_INT_EQ(-1, selected_id);
}

TEST(selection, delete_kosong_mengembalikan_nol) {
    test_reset_model();
    add_res_at(100, 100);
    clear_selection();
    ASSERT_INT_EQ(0, model_delete_selection());
    ASSERT_INT_EQ(1, comp_count);
}

TEST(selection, delete_dikunci_saat_mode_run) {
    test_reset_model();
    int r1 = add_res_at(100, 100);
    clear_selection();
    add_to_selection(r1);
    sim_set_running(1);
    ASSERT_INT_EQ(0, model_delete_selection());  /* edit dikunci saat RUN */
    sim_set_running(0);
    ASSERT_TRUE(find_comp_by_id(r1));            /* masih utuh */
}

/* ===================== model_duplicate_selection ===================== */

TEST(selection, duplicate_menyalin_blok_dan_kabel_internal) {
    test_reset_model();
    int r1 = add_res_at(100, 100);
    int r2 = add_res_at(200, 100);
    wire_ids(r1, 0, r2, 0);
    int luar = add_res_at(500, 500);
    wire_ids(r2, 1, luar, 0);                    /* kabel keluar: TIDAK ikut */

    clear_selection();
    add_to_selection(r1); add_to_selection(r2);

    int n = model_duplicate_selection(60, 60);
    ASSERT_INT_EQ(2, n);
    ASSERT_INT_EQ(4, comp_count);                /* 2 asli + 2 salinan */
    ASSERT_INT_EQ(3, wire_count);                /* 2 lama + 1 kabel internal baru */

    /* seleksi kini menunjuk ke salinan baru */
    ASSERT_INT_EQ(2, sel_count);
    Component *a = find_comp_by_id(selection[0]);
    Component *b = find_comp_by_id(selection[1]);
    ASSERT_TRUE(a && b);
    /* salinan bergeser tepat (60,60) dari aslinya, bentuk blok terjaga */
    int dx = b->x - a->x, dy = b->y - a->y;
    ASSERT_INT_EQ(100, dx);                      /* jarak horizontal asli */
    ASSERT_INT_EQ(0,   dy);

    /* kabel baru menghubungkan DUA SALINAN (bukan campuran asli/salinan) */
    int found_new_internal = 0;
    for (int i = 0; i < wire_count; i++) {
        int ai = wires[i].a.comp_id, bi = wires[i].b.comp_id;
        int a_in_sel = (ai == selection[0] || ai == selection[1]);
        int b_in_sel = (bi == selection[0] || bi == selection[1]);
        if (a_in_sel && b_in_sel) found_new_internal = 1;
    }
    ASSERT_INT_EQ(1, found_new_internal);
}

TEST(selection, duplicate_tanpa_seleksi_mengembalikan_nol) {
    test_reset_model();
    add_res_at(100, 100);
    clear_selection();
    ASSERT_INT_EQ(0, model_duplicate_selection(60, 60));
    ASSERT_INT_EQ(1, comp_count);
}

/* ===================== State group-drag (drag-and-drop blok) ============ */

TEST(selection, state_group_drag_terdefinisi_dapat_diubah) {
    test_reset_model();
    /* variabel global pendukung begin_group_drag/on_mouse_move di events.c */
    ASSERT_INT_EQ(0, group_dragging);
    ASSERT_INT_EQ(0, group_drag_n);

    group_dragging = 1;
    group_drag_items[0].idx = 3;
    group_drag_items[0].x0 = 100;
    group_drag_items[0].y0 = 200;
    group_drag_n = 1;
    group_drag_ox = 7; group_drag_oy = -9;

    ASSERT_INT_EQ(1, group_dragging);
    ASSERT_INT_EQ(1, group_drag_n);
    ASSERT_INT_EQ(3,   group_drag_items[0].idx);
    ASSERT_INT_EQ(100, group_drag_items[0].x0);
    ASSERT_INT_EQ(200, group_drag_items[0].y0);
    ASSERT_INT_EQ(7,   group_drag_ox);
    ASSERT_INT_EQ(-9,  group_drag_oy);

    /* reset simulasi/flag seperti clear_canvas harus mengembalikannya */
    model_clear_all();
    ASSERT_INT_EQ(0, group_dragging);
    ASSERT_INT_EQ(0, group_drag_n);
}

/* Skenario ujung-ke-ujung meniru on_mouse_move group drag:
   items[0] = anchor yang di-grab; seluruh anggota digeser dengan delta sama. */
TEST(selection, group_drag_delta_rigid_meniru_on_mouse_move) {
    test_reset_model();
    int r1 = add_res_at(100, 100);
    int r2 = add_res_at(140, 120);
    clear_selection();
    add_to_selection(r1); add_to_selection(r2);

    /* bangun tabel drag seperti begin_group_drag (anchor = r1) */
    group_drag_items[0].idx = (int)(find_comp_by_id(r1) - comps);
    group_drag_items[0].x0 = 100; group_drag_items[0].y0 = 100;
    group_drag_items[1].idx = (int)(find_comp_by_id(r2) - comps);
    group_drag_items[1].x0 = 140; group_drag_items[1].y0 = 120;
    group_drag_n = 2;
    group_drag_ox = 5; group_drag_oy = 5;   /* offset grab dalam pixel mentah */

    /* mouse pindah ke (265, 225) -> posisi ideal anchor snap ke (260? ) */
    int mx = 265, my = 225;
    int nx = snap(mx + group_drag_ox);      /* 270 -> grid 20 => 280? dicek via snap */
    int ny = snap(my + group_drag_oy);
    int dx = nx - group_drag_items[0].x0;
    int dy = ny - group_drag_items[0].y0;
    for (int i = 0; i < group_drag_n; i++) {
        comps[group_drag_items[i].idx].x = group_drag_items[i].x0 + dx;
        comps[group_drag_items[i].idx].y = group_drag_items[i].y0 + dy;
    }

    Component *c1 = find_comp_by_id(r1), *c2 = find_comp_by_id(r2);
    ASSERT_TRUE(c1 && c2);
    ASSERT_INT_EQ(nx, c1->x);               /* anchor mendarat di hasil snap */
    ASSERT_INT_EQ(ny, c1->y);
    /* rigid: vektor antar anggota tak berubah & tetap se-grid */
    ASSERT_INT_EQ(40, c2->x - c1->x);
    ASSERT_INT_EQ(20, c2->y - c1->y);
    ASSERT_INT_EQ(0, c1->x % GRID);
    ASSERT_INT_EQ(0, c2->x % GRID);
    ASSERT_INT_EQ(0, c1->y % GRID);
    ASSERT_INT_EQ(0, c2->y % GRID);

    group_drag_n = 0; group_dragging = 0;
}

/* ===================== Entry point ===================== */

void suite_selection(void) {
    SUITE_BEGIN("selection (multi-select / blok)");
    RUN(selection, add_menumpuk_id_unik_dalam_urutan_klik);
    RUN(selection, add_menolak_duplikat_id);
    RUN(selection, add_menolak_id_tak_valid);
    RUN(selection, toggle_memasukkan_lalu_mengeluarkan_id);
    RUN(selection, clear_mengosongkan_sel_count_dan_selected_id);
    RUN(selection, translate_gerakkan_semua_anggota_rigid);
    RUN(selection, translate_nol_dan_kosong_aman);
    RUN(selection, delete_hapus_seluruh_blok_dan_kabel_ikutannya);
    RUN(selection, delete_kosong_mengembalikan_nol);
    RUN(selection, delete_dikunci_saat_mode_run);
    RUN(selection, duplicate_menyalin_blok_dan_kabel_internal);
    RUN(selection, duplicate_tanpa_seleksi_mengembalikan_nol);
    RUN(selection, state_group_drag_terdefinisi_dapat_diubah);
    RUN(selection, group_drag_delta_rigid_meniru_on_mouse_move);
}
