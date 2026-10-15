/* test_model.c — Unit test untuk src/model.c
   Mencakup: snap(), find_comp_by_id/at, jumlah & posisi pin (rotasi),
   find_pin_at, operasi model (add/remove/clear/rotate/wire), serta
   utilitas nilai komponen (default, unit, parser, formatter). */

#include "schematic.h"
#include "tinytest.h"

void test_reset_model(void); /* dari tests/unit_stubs.c */

/* ===================== snap() ===================== */

TEST(model, snap_membulat_ke_grid_20) {
    ASSERT_INT_EQ(100, snap(100));      /* sudah pas di grid */
    ASSERT_INT_EQ(100, snap(98));       /* 98 -> 100 */
    ASSERT_INT_EQ(100, snap(104));      /* 104 -> 100 */
    ASSERT_INT_EQ(120, snap(105));      /* tie: round-half-away -> 120 */
    ASSERT_INT_EQ(0,    snap(0));
    ASSERT_INT_EQ(-20,  snap(-14));     /* negatif ikut membulat ke grid */
}

/* ===================== find_comp_by_id ===================== */

TEST(model, find_comp_by_id) {
    test_reset_model();
    model_add_component(COMP_RESISTOR, 200, 200);
    int id1 = comps[0].id;
    model_add_component(COMP_LED, 400, 200);
    int id2 = comps[1].id;

    Component *a = find_comp_by_id(id1);
    Component *b = find_comp_by_id(id2);
    ASSERT_NOT_NULL(a);
    ASSERT_NOT_NULL(b);
    ASSERT_INT_EQ(COMP_RESISTOR, a->type);
    ASSERT_INT_EQ(COMP_LED, b->type);
    ASSERT_TRUE(a != b);
    ASSERT_NULL(find_comp_by_id(9999));        /* id tak dikenal */
    ASSERT_NULL(find_comp_by_id(-5));          /* id negatif -> NULL, bukan crash */
    ASSERT_NULL(find_comp_by_id(0));           /* id mulai dari 1 (next_id) */
}

/* ===================== find_comp_at (hit-test badan) ===================== */

TEST(model, find_comp_at_hit_test) {
    test_reset_model();
    model_add_component(COMP_RESISTOR, 200, 200);   /* rot=0: hw=48, hh=38 */
    ASSERT_NOT_NULL(find_comp_at(200, 200));        /* pusat kena */
    ASSERT_NOT_NULL(find_comp_at(240, 220));        /* masih dalam kotak */
    ASSERT_NULL(find_comp_at(300, 300));            /* jauh -> miss */
    ASSERT_NULL(find_comp_at(152, 200));            /* tepat di luar hw=48 */

    /* tumpuk: komponen yang digambar terakhir (indeks besar) menang */
    model_add_component(COMP_LED, 200, 200);
    Component *top = find_comp_at(200, 200);
    ASSERT_NOT_NULL(top);
    ASSERT_INT_EQ(COMP_LED, top->type);
}

TEST(model, find_comp_at_rotasi_90_menukar_lebar_tinggi) {
    test_reset_model();
    model_add_component(COMP_RESISTOR, 200, 200);
    selected_id = comps[0].id;
    model_rotate_selected();                        /* rot: 0 -> 1 (90 CW) */
    ASSERT_INT_EQ(1, comps[0].rot);
    /* rot ganjil: hw=38, hh=48 -> titik (245,200) kini di luar, (200,240) di dalam */
    ASSERT_NULL(find_comp_at(245, 200));
    ASSERT_NOT_NULL(find_comp_at(200, 240));
}

/* ===================== Jumlah & validitas pin ===================== */

TEST(model, comp_pin_count_dan_validitas) {
    test_reset_model();
    Component g, r, q, n;
    memset(&g, 0, sizeof g); g.type = COMP_GROUND;
    memset(&r, 0, sizeof r); r.type = COMP_RESISTOR;
    memset(&q, 0, sizeof q); q.type = COMP_TRANSISTOR;
    memset(&n, 0, sizeof n); n.type = COMP_NONE;

    ASSERT_INT_EQ(1, comp_pin_count(&g));   /* ground cuma 1 pin */
    ASSERT_INT_EQ(2, comp_pin_count(&r));   /* dua-pin umum */
    ASSERT_INT_EQ(3, comp_pin_count(&q));   /* transistor C,E,B */
    ASSERT_INT_EQ(0, comp_pin_count(&n));
    ASSERT_INT_EQ(0, comp_pin_count(NULL)); /* tahan NULL */

    ASSERT_TRUE(comp_pin_valid(&r, 0));
    ASSERT_TRUE(comp_pin_valid(&r, 1));
    ASSERT_FALSE(comp_pin_valid(&r, 2));
    ASSERT_FALSE(comp_pin_valid(&r, -1));
    ASSERT_FALSE(comp_pin_valid(NULL, 0));
    ASSERT_TRUE(comp_pin_valid(&q, 2));     /* basis = pin2 sah utk transistor */
    ASSERT_FALSE(comp_pin_valid(&g, 1));    /* ground tak punya pin1 */
}

/* ===================== Transformasi lokal -> dunia (rotasi) ===================== */

TEST(model, comp_local_to_world_semua_rotasi) {
    test_reset_model();
    Component c;
    memset(&c, 0, sizeof c);
    c.type = COMP_RESISTOR; c.x = 100; c.y = 100;
    int wx, wy;

    c.rot = 0; comp_local_to_world(&c, 40, 0, &wx, &wy);
    ASSERT_INT_EQ(140, wx); ASSERT_INT_EQ(100, wy);

    c.rot = 1; comp_local_to_world(&c, 40, 0, &wx, &wy);   /* 90 CW: (x,y)->(-y,x) */
    ASSERT_INT_EQ(100, wx); ASSERT_INT_EQ(140, wy);

    c.rot = 2; comp_local_to_world(&c, 40, 0, &wx, &wy);   /* 180: (-x,-y) */
    ASSERT_INT_EQ(60, wx);  ASSERT_INT_EQ(100, wy);

    c.rot = 3; comp_local_to_world(&c, 40, 0, &wx, &wy);   /* 270 CW: (y,-x) */
    ASSERT_INT_EQ(100, wx); ASSERT_INT_EQ(60, wy);

    /* rot normalisasi: 5 ≡ 1, dan nilai negatif tidak boleh crash */
    c.rot = 5; comp_local_to_world(&c, 40, 0, &wx, &wy);
    ASSERT_INT_EQ(100, wx); ASSERT_INT_EQ(140, wy);
    c.rot = -3; comp_local_to_world(&c, 40, 0, &wx, &wy);  /* -3 ≡ 1 */
    ASSERT_INT_EQ(100, wx); ASSERT_INT_EQ(140, wy);

    /* tahan NULL tanpa menulis output */
    comp_local_to_world(NULL, 0, 0, &wx, &wy);
    comp_local_to_world(&c, 0, 0, NULL, NULL);
}

TEST(model, comp_pin_pos_kiri_kanan_dan_ground) {
    test_reset_model();
    Component c;
    memset(&c, 0, sizeof c);
    c.type = COMP_RESISTOR; c.x = 200; c.y = 200; c.rot = 0;
    int px, py;
    comp_pin_pos(&c, 0, &px, &py);
    ASSERT_INT_EQ(160, px); ASSERT_INT_EQ(200, py);       /* pin0 kiri */
    comp_pin_pos(&c, 1, &px, &py);
    ASSERT_INT_EQ(240, px); ASSERT_INT_EQ(200, py);       /* pin1 kanan */

    c.type = COMP_GROUND;
    comp_pin_pos(&c, 0, &px, &py);
    ASSERT_INT_EQ(200, px); ASSERT_INT_EQ(170, py);       /* (0,-30) */

    /* rotasi 90 derajat memutar posisi pin sesuai rumus dunia */
    c.type = COMP_RESISTOR; c.rot = 1;
    comp_pin_pos(&c, 0, &px, &py);
    ASSERT_INT_EQ(200, px); ASSERT_INT_EQ(160, py);       /* (-40,0)->(0,-40) */
}

TEST(model, comp_pin_pos_transistor_tiga_kaki) {
    test_reset_model();
    Component c;
    memset(&c, 0, sizeof c);
    c.type = COMP_TRANSISTOR; c.x = 300; c.y = 300; c.rot = 0;
    int px, py;
    comp_pin_pos(&c, 0, &px, &py);   /* kolektor  : (-40,-30) */
    ASSERT_INT_EQ(260, px); ASSERT_INT_EQ(270, py);
    comp_pin_pos(&c, 1, &px, &py);   /* emitor    : (+40,+30) */
    ASSERT_INT_EQ(340, px); ASSERT_INT_EQ(330, py);
    comp_pin_pos(&c, 2, &px, &py);   /* basis     : (-40,  0) */
    ASSERT_INT_EQ(260, px); ASSERT_INT_EQ(300, py);
}

/* ===================== find_pin_at ===================== */

TEST(model, find_pin_at_menemukan_pin_terdekat) {
    test_reset_model();
    model_add_component(COMP_RESISTOR, 200, 200);         /* pin0=(160,200) pin1=(240,200) */
    PinRef out; int px, py;
    out.comp_id = -1; out.pin_idx = -1;

    ASSERT_TRUE(find_pin_at(160, 200, &out, &px, &py));   /* tepat di pin0 */
    ASSERT_INT_EQ(comps[0].id, out.comp_id);
    ASSERT_INT_EQ(0, out.pin_idx);
    ASSERT_INT_EQ(160, px); ASSERT_INT_EQ(200, py);

    ASSERT_TRUE(find_pin_at(244, 203, &out, &px, &py));   /* dlm radius PIN_R+9 */
    ASSERT_INT_EQ(1, out.pin_idx);

    ASSERT_FALSE(find_pin_at(1000, 1000, &out, &px, &py));/* jauh -> tidak ada */

    /* varian argumen opsional NULL tidak boleh crash */
    ASSERT_TRUE(find_pin_at(160, 200, NULL, NULL, NULL));
}

/* ===================== model_add_component ===================== */

TEST(model, add_komponen_default_dan_state) {
    test_reset_model();
    model_add_component(COMP_NONE, 100, 100);             /* COMP_NONE ditolak */
    ASSERT_INT_EQ(0, comp_count);

    model_add_component(COMP_RESISTOR, 113, 127);         /* snap -> (120,120) */
    ASSERT_INT_EQ(1, comp_count);
    ASSERT_INT_EQ(2, next_id);                            /* id 1 sudah dikonsumsi */
    ASSERT_INT_EQ(1, comps[0].id);                        /* id pertama = 1 */
    ASSERT_INT_EQ(120, comps[0].x);
    ASSERT_INT_EQ(120, comps[0].y);
    ASSERT_INT_EQ(0, comps[0].rot);
    ASSERT_INT_EQ(0, comps[0].on);
    ASSERT_DBL_NEAR(220.0, comps[0].v, 1e-9);             /* default resistor */
    ASSERT_INT_EQ(comps[0].id, selected_id);              /* baru added = terpilih */

    model_add_component(COMP_LDR, 300, 300);              /* tipe baru dari commit LDR/Buzzer */
    ASSERT_INT_EQ(2, comp_count);
    ASSERT_DBL_NEAR(LDR_DARK_DEFAULT, comps[1].v, 1.0);   /* 1 MOhm */

    model_add_component(COMP_BUZZER, 500, 300);
    ASSERT_INT_EQ(3, comp_count);
    ASSERT_DBL_NEAR(BUZZER_FREQ_DEFAULT, comps[2].v, 1e-9); /* 1 kHz */
}

/* ===================== model_remove_at ===================== */

TEST(model, remove_komponen_membersihkan_seleksi_dan_kabel_yang_mengikut) {
    test_reset_model();
    model_add_component(COMP_RESISTOR, 200, 200);
    model_add_component(COMP_LED, 400, 200);
    int rid = comps[0].id, lid = comps[1].id;
    PinRef a = { rid, 1 }, b = { lid, 0 };
    ASSERT_INT_EQ(1, model_add_wire(a, b));
    ASSERT_INT_EQ(1, wire_count);

    selected_id = rid;
    model_remove_at(200, 200);                            /* hapus resistor */
    ASSERT_INT_EQ(1, comp_count);
    ASSERT_INT_EQ(0, wire_count);                         /* kabel ikut terhapus */
    ASSERT_INT_EQ(-1, selected_id);                       /* seleksi dibersihkan */
    ASSERT_NULL(find_comp_by_id(rid));
    ASSERT_NOT_NULL(find_comp_by_id(lid));

    /* remove di titik kosong = no-op */
    model_remove_at(1500, 1500);
    ASSERT_INT_EQ(1, comp_count);
}

TEST(model, remove_di_atas_kabel_menghapus_kabel_bukan_komponen) {
    test_reset_model();
    model_add_component(COMP_RESISTOR, 200, 200);
    model_add_component(COMP_LED, 400, 200);
    PinRef a = { comps[0].id, 1 }, b = { comps[1].id, 0 };
    model_add_wire(a, b);
    /* titik tengah segmen horizontal kabel: (240..240)-(360,200) corner (240,200) */
    ASSERT_INT_EQ(1, model_remove_wire_at(240, 200));     /* sanity: kabel kena */
    ASSERT_INT_EQ(0, wire_count);
    ASSERT_INT_EQ(2, comp_count);                         /* komponen utuh */
}

/* ===================== model_clear_all / rotate ===================== */

TEST(model, clear_all_mengosongkan_semua) {
    test_reset_model();
    model_add_component(COMP_SWITCH, 200, 200);
    model_add_component(COMP_BATTERY, 400, 200);
    comps[0].on = 1;
    PinRef a = { comps[0].id, 0 }, b = { comps[1].id, 0 };
    model_add_wire(a, b);
    model_clear_all();
    ASSERT_INT_EQ(0, comp_count);
    ASSERT_INT_EQ(0, wire_count);
    ASSERT_INT_EQ(-1, selected_id);
    ASSERT_INT_EQ(-1, dragging_id);
    ASSERT_INT_EQ(0, wiring_active);
}

TEST(model, rotate_selected_berputar_mod_4) {
    test_reset_model();
    model_add_component(COMP_RESISTOR, 200, 200);
    Component *c = &comps[0];
    for (int i = 1; i <= 4; i++) {
        selected_id = c->id;
        model_rotate_selected();
        ASSERT_INT_EQ(i % 4, c->rot);
    }
    /* tanpa seleksi -> tidak mengubah apa pun (tidak crash) */
    selected_id = -1;
    model_rotate_selected();
    ASSERT_INT_EQ(0, c->rot);
}

/* ===================== model_add_wire ===================== */

static void setup_two_comps(int *ida, int *idb) {
    test_reset_model();
    model_add_component(COMP_RESISTOR, 200, 200);
    model_add_component(COMP_LED, 400, 200);
    *ida = comps[0].id; *idb = comps[1].id;
}

TEST(model, wire_valid_ditambahkan) {
    int ia, ib; setup_two_comps(&ia, &ib);
    PinRef a = { ia, 1 }, b = { ib, 0 };
    ASSERT_INT_EQ(1, model_add_wire(a, b));
    ASSERT_INT_EQ(1, wire_count);
    ASSERT_INT_EQ(ia, wires[0].a.comp_id);
    ASSERT_INT_EQ(ib, wires[0].b.comp_id);
}

TEST(model, wire_tolak_id_negatif_sama_pin_dan_pin_tak_valid) {
    int ia, ib; setup_two_comps(&ia, &ib);
    PinRef bad1 = { -1, 0 }, ok = { ib, 0 };
    ASSERT_INT_EQ(0, model_add_wire(bad1, ok));           /* id < 0 */
    PinRef self = { ia, 1 };
    ASSERT_INT_EQ(0, model_add_wire(self, self));         /* pin sama -> self-loop ditolak */
    PinRef over = { ia, 5 };                              /* pin di luar jangkauan resistor */
    ASSERT_INT_EQ(0, model_add_wire(over, ok));
    PinRef ghost = { 9999, 0 };                           /* id tak dikenal */
    ASSERT_INT_EQ(0, model_add_wire(ghost, ok));
    ASSERT_INT_EQ(0, wire_count);
}

TEST(model, wire_duplikat_ditolak_termasuk_yang_dibalik) {
    int ia, ib; setup_two_comps(&ia, &ib);
    PinRef a = { ia, 1 }, b = { ib, 0 };
    ASSERT_INT_EQ(1, model_add_wire(a, b));
    ASSERT_INT_EQ(0, model_add_wire(a, b));               /* duplikat persis */
    ASSERT_INT_EQ(0, model_add_wire(b, a));               /* duplikat terbalik */
    ASSERT_INT_EQ(1, wire_count);
}

TEST(model, wire_transistor_pin_2_sah) {
    test_reset_model();
    model_add_component(COMP_TRANSISTOR, 200, 200);
    model_add_component(COMP_BATTERY, 400, 200);
    PinRef base = { comps[0].id, 2 };                     /* basis transistor */
    PinRef pos  = { comps[1].id, 0 };
    ASSERT_INT_EQ(1, model_add_wire(base, pos));          /* pin_idx 2 valid utk transistor */
}

/* ===================== Nilai komponen: default / has_value / unit ===================== */

TEST(model, comp_default_value_per_tipe) {
    ASSERT_DBL_NEAR(220.0,          comp_default_value(COMP_RESISTOR),   1e-9);
    ASSERT_DBL_NEAR(9.0,            comp_default_value(COMP_BATTERY),    1e-9);
    ASSERT_DBL_NEAR(220.0,          comp_default_value(COMP_ACSOURCE),   1e-9);
    ASSERT_DBL_NEAR(VF_LED_DEFAULT, comp_default_value(COMP_LED),        1e-9);
    ASSERT_DBL_NEAR(VF_DIODE,       comp_default_value(COMP_DIODE),      1e-9);
    ASSERT_DBL_NEAR(100e-9,         comp_default_value(COMP_CAPACITOR),  1e-15);
    ASSERT_DBL_NEAR(100e-6,         comp_default_value(COMP_CAPOLAR),    1e-12);
    ASSERT_DBL_NEAR(LDR_DARK_DEFAULT,   comp_default_value(COMP_LDR),    1.0);
    ASSERT_DBL_NEAR(BUZZER_FREQ_DEFAULT, comp_default_value(COMP_BUZZER), 1e-9);
    ASSERT_DBL_NEAR(0.0, comp_default_value(COMP_SWITCH), 1e-12);
    ASSERT_DBL_NEAR(0.0, comp_default_value(COMP_TRANSISTOR), 1e-12);
    ASSERT_DBL_NEAR(0.0, comp_default_value(COMP_GROUND), 1e-12);
    ASSERT_DBL_NEAR(0.0, comp_default_value(COMP_NONE), 1e-12);
}

TEST(model, comp_has_value) {
    ASSERT_TRUE(comp_has_value(COMP_RESISTOR));
    ASSERT_TRUE(comp_has_value(COMP_BATTERY));
    ASSERT_TRUE(comp_has_value(COMP_ACSOURCE));
    ASSERT_TRUE(comp_has_value(COMP_LED));
    ASSERT_TRUE(comp_has_value(COMP_DIODE));
    ASSERT_TRUE(comp_has_value(COMP_CAPACITOR));
    ASSERT_TRUE(comp_has_value(COMP_CAPOLAR));
    ASSERT_TRUE(comp_has_value(COMP_LDR));      /* tipe baru: value = R gelap */
    ASSERT_TRUE(comp_has_value(COMP_BUZZER));   /* tipe baru: value = frekuensi */
    ASSERT_FALSE(comp_has_value(COMP_SWITCH));
    ASSERT_FALSE(comp_has_value(COMP_TRANSISTOR));
    ASSERT_FALSE(comp_has_value(COMP_GROUND));
    ASSERT_FALSE(comp_has_value(COMP_NONE));
}

TEST(model, comp_value_unit) {
    ASSERT_TRUE(wcscmp(comp_value_unit(COMP_RESISTOR),  L"\x03A9") == 0); /* Ω */
    ASSERT_TRUE(wcscmp(comp_value_unit(COMP_BATTERY),   L"V")   == 0);
    ASSERT_TRUE(wcscmp(comp_value_unit(COMP_ACSOURCE),  L"VAC") == 0);
    ASSERT_TRUE(wcscmp(comp_value_unit(COMP_LED),       L"Vf")  == 0);
    ASSERT_TRUE(wcscmp(comp_value_unit(COMP_DIODE),     L"Vf")  == 0);
    ASSERT_TRUE(wcscmp(comp_value_unit(COMP_CAPACITOR), L"F")   == 0);
    ASSERT_TRUE(wcscmp(comp_value_unit(COMP_CAPOLAR),   L"F")   == 0);
    ASSERT_TRUE(wcscmp(comp_value_unit(COMP_LDR),       L"Rgelap") == 0);
    ASSERT_TRUE(wcscmp(comp_value_unit(COMP_BUZZER),    L"Hz")  == 0);
    ASSERT_NULL(comp_value_unit(COMP_SWITCH));
    ASSERT_NULL(comp_value_unit(COMP_GROUND));
    ASSERT_NULL(comp_value_unit(COMP_NONE));
}

/* ===================== Parser nilai ===================== */

static double pv(const wchar_t *s) {  /* helper: parse atau -1 bila gagal */
    double v = -1.0;
    return parse_value_text(s, &v) ? v : -1.0;
}

TEST(model, parse_value_angka_dasar) {
    ASSERT_DBL_NEAR(220.0, pv(L"220"), 1e-9);
    ASSERT_DBL_NEAR(4.7,   pv(L"4.7"), 1e-9);
    ASSERT_DBL_NEAR(0.5,   pv(L"0.5"), 1e-9);
    ASSERT_DBL_NEAR(2200.0, pv(L"2.2k"), 1e-9);
    ASSERT_DBL_NEAR(2200.0, pv(L"2.2K"), 1e-9);        /* K kapital */
    ASSERT_DBL_NEAR(1e6,   pv(L"1M"),  1e-3);          /* Mega */
    ASSERT_DBL_NEAR(1e-3,  pv(L"1m"),  1e-12);         /* milli */
    ASSERT_DBL_NEAR(100e-6, pv(L"100u"), 1e-12);
    ASSERT_DBL_NEAR(100e-6, pv(L"100\xB5"), 1e-12);    /* µ micro sign */
    ASSERT_DBL_NEAR(100e-6, pv(L"100\u03BC"), 1e-12);  /* μ greek mu */
    ASSERT_DBL_NEAR(47e-9,  pv(L"47n"), 1e-15);
    ASSERT_DBL_NEAR(47e-9,  pv(L"47N"), 1e-15);
    ASSERT_DBL_NEAR(22e-12, pv(L"22p"), 1e-18);
    ASSERT_DBL_NEAR(2.2e3,  pv(L"2.2e3"), 1e-9);       /* notasi ilmiah */
}

TEST(model, parse_value_satuan_gabung_dan_spasi) {
    ASSERT_DBL_NEAR(47e-6, pv(L"47uF"), 1e-12);        /* prefiks + satuan */
    ASSERT_DBL_NEAR(100e-9, pv(L"100n F"), 1e-15);     /* spasi sebelum satuan */
    ASSERT_DBL_NEAR(10000.0, pv(L" 10k "), 1e-9);      /* spasi tepi */
    ASSERT_DBL_NEAR(220.0, pv(L"220 \x03A9"), 1e-9);   /* "220 Ω" */
    ASSERT_DBL_NEAR(220.0, pv(L"220Ohm"), 1e-9);       /* kata Ohm */
    ASSERT_DBL_NEAR(9.0,   pv(L"9V"), 1e-9);           /* volt */
    ASSERT_DBL_NEAR(0.7,   pv(L"0.7v"), 1e-9);
}

TEST(model, parse_value_menolak_input_sah_tapi_berubah_nilai) {
    /* kasus yang HARUS ditolak (return 0): */
    double v = 123.0;
    ASSERT_FALSE(parse_value_text(L"", &v));           /* kosong */
    ASSERT_FALSE(parse_value_text(L"abc", &v));        /* bukan angka */
    ASSERT_FALSE(parse_value_text(L"-5", &v));         /* negatif */
    ASSERT_FALSE(parse_value_text(L"nan", &v));        /* NaN ditolak via !(v>=0) */
    ASSERT_FALSE(parse_value_text(L"10kk", &v));       /* prefiks ganda */
    ASSERT_FALSE(parse_value_text(L"10x", &v));        /* karakter asing */
    ASSERT_FALSE(parse_value_text(L"1 e3", &v));       /* 'e' terpisah -> tolak */
    ASSERT_INT_EQ(123, (int)v);                        /* out tidak ditulis saat gagal */

    /* NOTE(parser-karakteristik): wcstod menerima hex-float & inf;
       ini perilaku terdokumentasi dari implementasi saat ini. */
    ASSERT_TRUE(parse_value_text(L"inf", &v));
    ASSERT_TRUE(v > 1e300);
    ASSERT_TRUE(parse_value_text(L"0x10", &v));
    ASSERT_DBL_NEAR(16.0, v, 1e-9);
}

TEST(model, parse_value_null_args) {
    double v = 0;
    ASSERT_FALSE(parse_value_text(NULL, &v));
    ASSERT_FALSE(parse_value_text(L"10", NULL));
    ASSERT_FALSE(parse_value_text_a(NULL, &v));
}

TEST(model, parse_value_text_a_varian_ascii) {
    double v = -1;
    ASSERT_TRUE(parse_value_text_a("2.2k", &v));
    ASSERT_DBL_NEAR(2200.0, v, 1e-9);
    ASSERT_TRUE(parse_value_text_a("47uF", &v));
    ASSERT_DBL_NEAR(47e-6, v, 1e-12);
    ASSERT_FALSE(parse_value_text_a("xx", &v));
    /* string panjang dipotong ke 63 char namun tetap terparse sebagai angka */
    char big[128];
    memset(big, '9', 100); big[100] = 0;
    ASSERT_TRUE(parse_value_text_a(big, &v));
}

/* ===================== Formatter nilai ===================== */

static void fmt(double v, const wchar_t *unit, wchar_t *out, int cap) {
    format_value(v, unit, out, cap);
}

TEST(model, format_value_prefiks_si) {
    wchar_t o[64];
    fmt(220.0, L"\x03A9", o, 64);
    ASSERT_TRUE(wcscmp(o, L"220 \x03A9") == 0);            /* "220 Ω" */
    fmt(2200.0, L"\x03A9", o, 64);
    ASSERT_TRUE(wcscmp(o, L"2.2 k\x03A9") == 0);           /* "2.2 kΩ" */
    fmt(1e6, L"\x03A9", o, 64);
    ASSERT_TRUE(wcscmp(o, L"1 M\x03A9") == 0);             /* "1 MΩ" */
    fmt(9.0, L"V", o, 64);
    ASSERT_TRUE(wcscmp(o, L"9 V") == 0);                   /* integer tanpa desimal */
    fmt(100e-9, L"F", o, 64);
    ASSERT_TRUE(wcscmp(o, L"100 nF") == 0);
    fmt(47e-6, L"F", o, 64);
    ASSERT_TRUE(wcscmp(o, L"47 uF") == 0);
    fmt(1e-3, L"A", o, 64);
    ASSERT_TRUE(wcscmp(o, L"1 mA") == 0);
    fmt(0.0, L"V", o, 64);
    ASSERT_TRUE(wcscmp(o, L"0 V") == 0);                   /* nol -> tanpa prefiks */
    fmt(-2200.0, L"\x03A9", o, 64);
    ASSERT_TRUE(wcscmp(o, L"-2.2 k\x03A9") == 0);          /* negatif pakai |v| utk skala */
    fmt(1000.0, NULL, o, 64);
    ASSERT_TRUE(wcscmp(o, L"1 k") == 0);                   /* unit NULL aman */
    fmt(100000000.0, L"Hz", o, 64);
    ASSERT_TRUE(wcscmp(o, L"100 MHz") == 0);               /* frekuensi buzzer */
}

/* ===================== State global & toolbar table ===================== */

TEST(model, btn_tools_dan_labels_konsisten) {
    ASSERT_INT_EQ(BTN_COUNT, 15);
    /* urutan palette dari commit terbaru: ... LDR, Buzzer, Delete */
    ASSERT_INT_EQ(TOOL_SELECT,    btnTools[0]);
    ASSERT_INT_EQ(TOOL_WIRE,      btnTools[1]);
    ASSERT_INT_EQ(TOOL_LDR,       btnTools[12]);
    ASSERT_INT_EQ(TOOL_BUZZER,    btnTools[13]);
    ASSERT_INT_EQ(TOOL_DELETE,    btnTools[BTN_COUNT - 1]);
    /* semua label terisi (bukan NULL) agar tooltip/status tidak crash */
    for (int i = 0; i < BTN_COUNT; i++) {
        ASSERT_NOT_NULL(btnLabels[i]);
        ASSERT_TRUE(wcslen(btnLabels[i]) > 0);
    }
    /* indeks tombol aksi berada setelah tombol tool */
    ASSERT_TRUE(CLEAR_BTN_INDEX >= BTN_COUNT);
    ASSERT_TRUE(RUN_BTN_INDEX == CLEAR_BTN_INDEX + 3);
}

/* Entry point suite ini (dipanggil dari test_main.c). */
void suite_model(void) {
    SUITE_BEGIN("model.c");
    RUN(model, snap_membulat_ke_grid_20);
    RUN(model, find_comp_by_id);
    RUN(model, find_comp_at_hit_test);
    RUN(model, find_comp_at_rotasi_90_menukar_lebar_tinggi);
    RUN(model, comp_pin_count_dan_validitas);
    RUN(model, comp_local_to_world_semua_rotasi);
    RUN(model, comp_pin_pos_kiri_kanan_dan_ground);
    RUN(model, comp_pin_pos_transistor_tiga_kaki);
    RUN(model, find_pin_at_menemukan_pin_terdekat);
    RUN(model, add_komponen_default_dan_state);
    RUN(model, remove_komponen_membersihkan_seleksi_dan_kabel_yang_mengikut);
    RUN(model, remove_di_atas_kabel_menghapus_kabel_bukan_komponen);
    RUN(model, clear_all_mengosongkan_semua);
    RUN(model, rotate_selected_berputar_mod_4);
    RUN(model, wire_valid_ditambahkan);
    RUN(model, wire_tolak_id_negatif_sama_pin_dan_pin_tak_valid);
    RUN(model, wire_duplikat_ditolak_termasuk_yang_dibalik);
    RUN(model, wire_transistor_pin_2_sah);
    RUN(model, comp_default_value_per_tipe);
    RUN(model, comp_has_value);
    RUN(model, comp_value_unit);
    RUN(model, parse_value_angka_dasar);
    RUN(model, parse_value_satuan_gabung_dan_spasi);
    RUN(model, parse_value_menolak_input_sah_tapi_berubah_nilai);
    RUN(model, parse_value_null_args);
    RUN(model, parse_value_text_a_varian_ascii);
    RUN(model, format_value_prefiks_si);
    RUN(model, btn_tools_dan_labels_konsisten);
}
