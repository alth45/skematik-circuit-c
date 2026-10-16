/* test_sim.c — Unit test untuk src/sim.c (analisis rangkaian saat RUN)
   Mencakup: deteksi korslet, LED nyala + hukum Ohm, overcurrent/meledak,
   dioda arah, transistor NPN, kapasitor (blokir DC / tembus AC), sumber AC,
   hot-wire, pesan status, formatter SI, serta kontrak fungsi LDR/Buzzer
   (sim_ldr_effective_r / sim_buzzer_on — implementasi produksi di src/sim.c,
   TIDAK ada stub/seam: tes berjalan langsung terhadap kode yang dibuild). */

#include "schematic.h"
#include "tinytest.h"

/* test_reset_model() dideklarasikan di schematic.h (blok SCHEMATIC_TEST_STUBS);
   realisasinya di tests/test_support.c — berkas penunjang murni tes. */

/* ---------- Helper perakit rangkaian ---------- */
static Component *add_at(CompType t, int x, int y) {
    model_add_component(t, x, y);
    return &comps[comp_count - 1];
}
static void wire_pins(Component *a, int pa, Component *b, int pb) {
    PinRef x = { a->id, pa }, y = { b->id, pb };
    model_add_wire(x, y);
}
/* Baterai -> (saklar?) -> R -> LED -> GND; saklar disisipkan bila out_sw valid. */
static void build_basic_loop(double r_ohm, Component **out_bat, Component **out_led,
                             Component **out_sw) {
    Component *bat = add_at(COMP_BATTERY, 200, 200);
    Component *r   = add_at(COMP_RESISTOR, 400, 200);
    Component *led = add_at(COMP_LED,     600, 200);
    Component *gnd = add_at(COMP_GROUND,  800, 200);
    r->v = r_ohm;
    if (out_sw && *out_sw) {
        Component *sw = *out_sw;
        wire_pins(bat, 0, sw, 0);
        wire_pins(sw, 1, r, 0);
    } else {
        wire_pins(bat, 0, r, 0);
    }
    wire_pins(r, 1, led, 0);
    wire_pins(led, 1, gnd, 0);
    wire_pins(bat, 1, gnd, 0);
    if (out_bat) *out_bat = bat;
    if (out_led) *out_led = led;
}

/* Rakit ulang loop dengan posisi saklar berbeda: komponen yang sudah dibuat
   sebelumnya dipakai kembali (posisi x,y-nya tidak memengaruhi analisis). */
static void rebuild_loop_with_switch(Component *bat, Component *sw, Component *r,
                                     Component *led, Component *gnd) {
    wire_pins(bat, 0, sw, 0);
    wire_pins(sw, 1, r, 0);
    wire_pins(r, 1, led, 0);
    wire_pins(led, 1, gnd, 0);
    wire_pins(bat, 1, gnd, 0);
}

/* ===================== State awal & sumber ===================== */

TEST(sim, canvas_kosong_dan_tanpa_sumber) {
    test_reset_model();
    sim_recompute();
    ASSERT_INT_EQ(0, sim_short);
    ASSERT_INT_EQ(0, sim_has_battery);
    ASSERT_INT_EQ(0, sim_has_ac);
    ASSERT_INT_EQ(0, sim_led_count);
    ASSERT_INT_EQ(0, sim_running);
    ASSERT_WSTR_HAS(sim_message(), L"Belum ada sumber");

    add_at(COMP_LED, 200, 200);              /* LED saja, tanpa baterai */
    sim_recompute();
    ASSERT_INT_EQ(0, sim_led_count);
    ASSERT_WSTR_HAS(sim_message(), L"Belum ada sumber");
}

TEST(sim, loop_benar_led_menyala_hukum_ohm) {
    test_reset_model();
    Component *bat, *led;
    build_basic_loop(350.0, &bat, &led, NULL);   /* (9 - 2) / 350 = 20 mA */
    sim_recompute();
    ASSERT_INT_EQ(1, sim_has_battery);
    ASSERT_INT_EQ(1, sim_led_count);
    ASSERT_INT_EQ(1, sim_comp_lit(led));
    ASSERT_INT_EQ(0, sim_exploded);
    ASSERT_INT_EQ(0, sim_overcur);
    ASSERT_INT_EQ(0, sim_short);
    ASSERT_DBL_NEAR(9.0, sim_vsrc, 1e-9);
    ASSERT_DBL_NEAR(350.0, sim_rloop, 1e-6);
    ASSERT_DBL_NEAR(0.02, sim_comp_current(led), 1e-9);
    ASSERT_WSTR_HAS(sim_message(), L"LED MENYALA");
    ASSERT_WSTR_HAS(sim_message(), L"Vbat");
    /* komponen non-LED tidak punya arus hasil hitung */
    ASSERT_DBL_NEAR(-1.0, sim_comp_current(bat), 1e-12);
}

TEST(sim, saklar_off_memutus_led_on_menghubungkan) {
    test_reset_model();
    Component *sw  = add_at(COMP_SWITCH, 300, 200);
    Component *bat = add_at(COMP_BATTERY, 100, 200);
    Component *r   = add_at(COMP_RESISTOR, 500, 200);
    Component *led = add_at(COMP_LED, 700, 200);
    Component *gnd = add_at(COMP_GROUND, 900, 200);
    r->v = 350.0;
    rebuild_loop_with_switch(bat, sw, r, led, gnd);
    sw->on = 0;
    sim_recompute();
    ASSERT_INT_EQ(0, sim_led_count);
    ASSERT_INT_EQ(0, sim_comp_lit(led));
    ASSERT_WSTR_HAS(sim_message(), L"belum lengkap");

    sw->on = 1;                                /* tuas tertutup */
    sim_recompute();
    ASSERT_INT_EQ(1, sim_led_count);
    ASSERT_INT_EQ(1, sim_comp_lit(led));
    ASSERT_DBL_NEAR(0.02, sim_comp_current(led), 1e-9);
}

TEST(sim, polaritas_terbalik_led_mati) {
    test_reset_model();
    Component *bat = add_at(COMP_BATTERY, 200, 200);
    Component *r   = add_at(COMP_RESISTOR, 400, 200);
    Component *led = add_at(COMP_LED, 600, 200);
    Component *gnd = add_at(COMP_GROUND, 800, 200);
    r->v = 350.0;
    /* sengaja terbalik: (-) baterai -> anoda LED, (+) -> ground/katoda */
    wire_pins(bat, 1, r, 0);
    wire_pins(r, 1, led, 0);
    wire_pins(led, 1, gnd, 0);
    wire_pins(bat, 0, gnd, 0);
    sim_recompute();
    ASSERT_INT_EQ(0, sim_led_count);
    ASSERT_INT_EQ(0, sim_comp_lit(led));
}

/* ===================== Korsleting ===================== */

TEST(sim, korslet_langsung_plus_ke_minus) {
    test_reset_model();
    Component *bat = add_at(COMP_BATTERY, 200, 200);
    Component *gnd = add_at(COMP_GROUND, 400, 200);
    wire_pins(bat, 0, gnd, 0);                    /* (+) langsung ke (-) */
    sim_recompute();
    ASSERT_INT_EQ(1, sim_short);
    ASSERT_WSTR_HAS(sim_message(), L"KORSLETING");
}

TEST(sim, korslet_lewat_saklar_on) {
    test_reset_model();
    Component *bat = add_at(COMP_BATTERY, 200, 200);
    Component *sw  = add_at(COMP_SWITCH, 400, 200);
    Component *gnd = add_at(COMP_GROUND, 600, 200);
    wire_pins(bat, 0, sw, 0);
    wire_pins(sw, 1, gnd, 0);
    wire_pins(bat, 1, gnd, 0);
    sw->on = 0;
    sim_recompute();
    ASSERT_INT_EQ(0, sim_short);                /* saklar terbuka: aman */
    sw->on = 1;
    sim_recompute();
    ASSERT_INT_EQ(1, sim_short);                /* saklar tertutup: korslet */
}

/* ===================== Nasib LED: overcurrent & meledak ===================== */

TEST(sim, led_overcurrent_rusak_pelan) {
    test_reset_model();
    Component *bat, *led;
    build_basic_loop(300.0, &bat, &led, NULL);  /* I = 7/300 ≈ 23.3 mA > 20 mA */
    sim_recompute();
    ASSERT_INT_EQ(1, sim_led_count);
    ASSERT_INT_EQ(1, sim_overcur);
    ASSERT_INT_EQ(0, sim_exploded);
    ASSERT_INT_EQ(1, sim_comp_blown(led));      /* rusak pelan tetap dicatat */
    ASSERT_WSTR_HAS(sim_message(), L"rusak pelan");
}

TEST(sim, led_meledak_tanpa_resistor) {
    test_reset_model();
    Component *bat = add_at(COMP_BATTERY, 200, 200);
    Component *led = add_at(COMP_LED, 400, 200);
    Component *gnd = add_at(COMP_GROUND, 600, 200);
    wire_pins(bat, 0, led, 0);
    wire_pins(led, 1, gnd, 0);
    wire_pins(bat, 1, gnd, 0);
    sim_recompute();
    ASSERT_INT_EQ(1, sim_exploded);
    ASSERT_INT_EQ(1, sim_comp_blown(led));
    ASSERT_INT_EQ(1, sim_comp_lit(led));        /* "menyala terang" sbg indikasi */
    ASSERT_TRUE(sim_comp_current(led) >= 1e8);  /* arus ~ tak hingga */
    ASSERT_WSTR_HAS(sim_message(), L"MELEDAK");
}

TEST(sim, led_meledak_arus_di_atas_5x_if_max) {
    test_reset_model();
    Component *bat, *led;
    build_basic_loop(50.0, &bat, &led, NULL);   /* I = 7/50 = 140 mA > 100 mA */
    sim_recompute();
    ASSERT_INT_EQ(1, sim_exploded);
    ASSERT_INT_EQ(0, sim_overcur);              /* jalur ledak, bukan peringatan */
    ASSERT_WSTR_HAS(sim_message(), L"MELEDAK");
}

TEST(sim, vled_ge_vsumber_led_tidak_menyala_terang) {
    test_reset_model();
    Component *bat = add_at(COMP_BATTERY, 200, 200);
    Component *r   = add_at(COMP_RESISTOR, 400, 200);
    Component *led = add_at(COMP_LED, 600, 200);
    Component *gnd = add_at(COMP_GROUND, 800, 200);
    bat->v = 1.5;                               /* < Vf LED 2.0 V */
    r->v = 350.0;
    wire_pins(bat, 0, r, 0);
    wire_pins(r, 1, led, 0);
    wire_pins(led, 1, gnd, 0);
    wire_pins(bat, 1, gnd, 0);
    sim_recompute();
    ASSERT_INT_EQ(1, sim_led_count);            /* secara topologi tersambung */
    ASSERT_DBL_NEAR(0.0, sim_comp_current(led), 1e-12); /* tapi I = 0 (dv <= 0) */
    ASSERT_INT_EQ(0, sim_exploded);
    ASSERT_INT_EQ(0, sim_overcur);
}

/* ===================== Dioda ===================== */

TEST(sim, dioda_forward_menghantar_reverse_memblokir) {
    test_reset_model();
    Component *d1 = add_at(COMP_DIODE, 200, 200);   /* anoda ke (+): forward */
    Component *d2 = add_at(COMP_DIODE, 200, 400);   /* katoda ke (+): reverse */
    Component *bat = add_at(COMP_BATTERY, 500, 200);
    Component *gnd = add_at(COMP_GROUND, 500, 400);
    wire_pins(bat, 0, d1, 0);                          /* (+) -> A1 */
    wire_pins(d1, 1, gnd, 0);                          /* K1 -> (-) : korslet via dioda */
    wire_pins(bat, 0, d2, 1);                          /* (+) -> K2 (terbalik) */
    wire_pins(d2, 0, gnd, 0);                          /* A2 -> (-) */
    wire_pins(bat, 1, gnd, 0);
    sim_recompute();
    ASSERT_INT_EQ(1, sim_diode_fwd(d1));
    ASSERT_INT_EQ(0, sim_diode_fwd(d2));
    /* tipe lain selalu 0 dari sim_diode_fwd */
    ASSERT_INT_EQ(0, sim_diode_fwd(bat));
    ASSERT_INT_EQ(0, sim_diode_fwd(NULL));
}

TEST(sim, dioda_seri_menambah_vdrop) {
    test_reset_model();
    Component *bat = add_at(COMP_BATTERY, 200, 200);
    Component *d   = add_at(COMP_DIODE, 350, 200);
    Component *r   = add_at(COMP_RESISTOR, 500, 200);
    Component *led = add_at(COMP_LED, 650, 200);
    Component *gnd = add_at(COMP_GROUND, 800, 200);
    r->v = 350.0;
    wire_pins(bat, 0, d, 0);       /* (+) -> A */
    wire_pins(d, 1, r, 0);         /* K -> R */
    wire_pins(r, 1, led, 0);
    wire_pins(led, 1, gnd, 0);
    wire_pins(bat, 1, gnd, 0);
    sim_recompute();
    ASSERT_INT_EQ(1, sim_diode_fwd(d));
    ASSERT_INT_EQ(1, sim_led_count);
    /* I = (9 - Vf_led 2 - Vf_diode 0.7) / 350 = 6.3/350 = 18 mA */
    ASSERT_DBL_NEAR(0.018, sim_comp_current(led), 1e-6);
    ASSERT_INT_EQ(0, sim_overcur);
}

/* ===================== Transistor NPN ===================== */

TEST(sim, transistor_on_mengalirkan_led) {
    test_reset_model();
    Component *bat = add_at(COMP_BATTERY, 100, 200);
    Component *rb  = add_at(COMP_RESISTOR, 250, 200);
    Component *q   = add_at(COMP_TRANSISTOR, 400, 200);
    Component *rc  = add_at(COMP_RESISTOR, 550, 200);
    Component *led = add_at(COMP_LED, 700, 200);
    Component *gnd = add_at(COMP_GROUND, 850, 200);
    rb->v = 1000.0; rc->v = 350.0;
    wire_pins(bat, 0, rb, 0);      /* (+) -> basis lewat Rb */
    wire_pins(rb, 1, q, 2);        /* B */
    wire_pins(q, 1, gnd, 0);       /* E -> (-) */
    wire_pins(bat, 1, gnd, 0);     /* (-) -> gnd */
    wire_pins(bat, 0, rc, 0);      /* (+) -> kolektor lewat Rc */
    wire_pins(rc, 1, led, 0);
    wire_pins(led, 1, gnd, 0);
    sim_recompute();
    ASSERT_INT_EQ(1, sim_comp_conducting(q));
    ASSERT_INT_EQ(1, sim_led_count);
    /* jalur LED: Rc saja (Rb tak dilewati pencarian jalur) ; drop = 2 + 0.2 */
    ASSERT_DBL_NEAR((9.0 - 2.2) / 350.0, sim_comp_current(led), 1e-6);
    /* tipe lain selalu 0 */
    ASSERT_INT_EQ(0, sim_comp_conducting(bat));
    ASSERT_INT_EQ(0, sim_comp_conducting(NULL));
}

TEST(sim, transistor_off_basis_mengambang) {
    test_reset_model();
    Component *bat = add_at(COMP_BATTERY, 100, 200);
    Component *q   = add_at(COMP_TRANSISTOR, 400, 200);
    Component *rc  = add_at(COMP_RESISTOR, 550, 200);
    Component *led = add_at(COMP_LED, 700, 200);
    Component *gnd = add_at(COMP_GROUND, 850, 200);
    rc->v = 350.0;
    wire_pins(q, 1, gnd, 0);
    wire_pins(bat, 1, gnd, 0);
    wire_pins(bat, 0, rc, 0);
    wire_pins(rc, 1, led, 0);
    wire_pins(led, 1, gnd, 0);
    /* basis (pin2) tidak terhubung ke mana-mana */
    sim_recompute();
    ASSERT_INT_EQ(0, sim_comp_conducting(q));
    ASSERT_INT_EQ(0, sim_led_count);
}

/* ===================== Kapasitor & sumber AC ===================== */

TEST(sim, kapasitor_memblokir_dc) {
    test_reset_model();
    Component *bat = add_at(COMP_BATTERY, 200, 200);
    Component *cap = add_at(COMP_CAPACITOR, 400, 200);
    Component *led = add_at(COMP_LED, 600, 200);
    Component *gnd = add_at(COMP_GROUND, 800, 200);
    wire_pins(bat, 0, cap, 0);
    wire_pins(cap, 1, led, 0);
    wire_pins(led, 1, gnd, 0);
    wire_pins(bat, 1, gnd, 0);
    sim_recompute();
    ASSERT_INT_EQ(0, sim_led_count);            /* DC terblokir */
    ASSERT_INT_EQ(0, sim_ac_mode);
}

TEST(sim, sumber_ac_menyalakan_led_mode_rms) {
    test_reset_model();
    Component *ac  = add_at(COMP_ACSOURCE, 200, 200);
    Component *r   = add_at(COMP_RESISTOR, 400, 200);
    Component *led = add_at(COMP_LED, 600, 200);
    ac->v = 12.0;  r->v = 500.0;                /* (12-2)/500 = 20 mA tepat */
    wire_pins(ac, 0, r, 0);
    wire_pins(r, 1, led, 0);
    wire_pins(led, 1, ac, 1);                     /* Netral sebagai (-) */
    sim_recompute();
    ASSERT_INT_EQ(1, sim_has_ac);
    ASSERT_INT_EQ(0, sim_has_battery);
    ASSERT_INT_EQ(1, sim_ac_mode);
    ASSERT_INT_EQ(1, sim_led_count);
    ASSERT_DBL_NEAR(12.0, sim_vsrc, 1e-9);
    ASSERT_DBL_NEAR(0.02, sim_comp_current(led), 1e-9);
    ASSERT_INT_EQ(0, sim_exploded);             /* 20 mA tidak > 20 mA */
    ASSERT_WSTR_HAS(sim_message(), L"AC BENAR");
}

TEST(sim, ac_220v_tanpa_resistor_meledak_dengan_peringatan) {
    test_reset_model();
    Component *ac  = add_at(COMP_ACSOURCE, 200, 200);   /* default 220 V RMS */
    Component *r   = add_at(COMP_RESISTOR, 400, 200);
    Component *led = add_at(COMP_LED, 600, 200);
    r->v = 100.0;                                        /* I = 218/100 = 2.18 A */
    wire_pins(ac, 0, r, 0);
    wire_pins(r, 1, led, 0);
    wire_pins(led, 1, ac, 1);
    sim_recompute();
    ASSERT_INT_EQ(1, sim_exploded);
    ASSERT_WSTR_HAS(sim_message(), L"MELEDAK");
}

/* REGRESI: sumber AC 220 V pada rangkaian TERPISAH tidak boleh membuat
   loop baterai+R+LED yang sudah aman dituduh "MELEDAK". Perhitungan harus
   memakai tegangan sumber pada LOOP LED itu sendiri (KVL), bukan maksimum
   global seluruh rangkaian. */
TEST(sim, sumber_ac_terpisah_tidak_membuat_loop_dc_yang_aman_meledak) {
    test_reset_model();
    /* Loop DC ideal: (9 - 2) / 350 = 20 mA -> AMAN */
    Component *bat = add_at(COMP_BATTERY, 100, 100);
    Component *r   = add_at(COMP_RESISTOR, 300, 100);
    Component *led = add_at(COMP_LED, 500, 100);
    Component *gnd = add_at(COMP_GROUND, 700, 100);
    r->v = 350.0;
    wire_pins(bat, 0, r, 0);
    wire_pins(r, 1, led, 0);
    wire_pins(led, 1, gnd, 0);
    wire_pins(bat, 1, gnd, 0);
    sim_recompute();
    ASSERT_INT_EQ(0, sim_exploded);
    ASSERT_INT_EQ(0, sim_overcur);

    /* Tambahkan AC 220 V + buzzer pada rangkaian lain (tak terhubung) */
    Component *ac  = add_at(COMP_ACSOURCE, 100, 400);
    Component *buz = add_at(COMP_BUZZER, 300, 400);
    ac->v = 220.0;
    wire_pins(ac, 0, buz, 0);
    wire_pins(buz, 1, ac, 1);
    sim_recompute();
    ASSERT_INT_EQ(1, sim_led_count);            /* LED DC tetap menyala */
    ASSERT_INT_EQ(1, sim_comp_lit(led));
    ASSERT_INT_EQ(0, sim_exploded);             /* BUKAN meledak */
    ASSERT_INT_EQ(0, sim_overcur);
    ASSERT_DBL_NEAR(9.0, sim_vsrc, 1e-9);       /* sumber loop = baterai 9 V */
    ASSERT_DBL_NEAR(0.02, sim_comp_current(led), 1e-9);
    ASSERT_WSTR_HAS(sim_message(), L"BENAR");
}

/* REGRESI: dua baterai berbeda nilai pada loop berbeda — tiap LED dihitung
   dengan sumber pada loop-nya sendiri. */
TEST(sim, dua_loop_sumber_berbeda_nilai_terpisah) {
    test_reset_model();
    /* Loop 1: 9V, R=350 -> 20 mA (aman) */
    Component *bat1 = add_at(COMP_BATTERY, 100, 100);
    Component *r1   = add_at(COMP_RESISTOR, 300, 100);
    Component *led1 = add_at(COMP_LED, 500, 100);
    bat1->v = 9.0; r1->v = 350.0;
    wire_pins(bat1, 0, r1, 0);
    wire_pins(r1, 1, led1, 0);
    wire_pins(led1, 1, bat1, 1);
    /* Loop 2: 4.5V, R=25 -> (4.5-2)/25 = 100 mA -> overcurrent (bukan ledak) */
    Component *bat2 = add_at(COMP_BATTERY, 100, 400);
    Component *r2   = add_at(COMP_RESISTOR, 300, 400);
    Component *led2 = add_at(COMP_LED, 500, 400);
    bat2->v = 4.5; r2->v = 25.0;
    wire_pins(bat2, 0, r2, 0);
    wire_pins(r2, 1, led2, 0);
    wire_pins(led2, 1, bat2, 1);
    sim_recompute();
    ASSERT_INT_EQ(2, sim_led_count);
    ASSERT_INT_EQ(0, sim_exploded);             /* tak ada sumber >50V di loop */
    ASSERT_INT_EQ(1, sim_overcur);              /* hanya LED loop-4.5V */
    ASSERT_INT_EQ(0, sim_comp_blown(led1));     /* LED 9V/350ohm tetap aman */
    ASSERT_INT_EQ(1, sim_comp_blown(led2));
    ASSERT_DBL_NEAR(0.02, sim_comp_current(led1), 1e-9);
    ASSERT_DBL_NEAR(0.1, sim_comp_current(led2), 1e-9);
}

TEST(sim, kapasitor_tembus_dalam_mode_ac) {
    test_reset_model();
    Component *ac  = add_at(COMP_ACSOURCE, 200, 200);
    Component *cap = add_at(COMP_CAPACITOR, 400, 200);
    Component *r   = add_at(COMP_RESISTOR, 600, 200);
    Component *led = add_at(COMP_LED, 800, 200);
    ac->v = 12.0; r->v = 500.0;
    wire_pins(ac, 0, cap, 0);                    /* AC -> C (tembus di mode AC) */
    wire_pins(cap, 1, r, 0);
    wire_pins(r, 1, led, 0);
    wire_pins(led, 1, ac, 1);
    sim_recompute();
    ASSERT_INT_EQ(1, sim_ac_mode);
    ASSERT_INT_EQ(1, sim_led_count);           /* loop AC tertutup via kapasitor */
    ASSERT_DBL_NEAR(0.02, sim_comp_current(led), 1e-9);
}

/* ===================== Kabel panas (hot wire) ===================== */

TEST(sim, sim_wire_hot_dan_guard_indeks) {
    test_reset_model();
    Component *bat = add_at(COMP_BATTERY, 200, 200);
    Component *r   = add_at(COMP_RESISTOR, 400, 200);
    Component *gnd = add_at(COMP_GROUND, 600, 200);
    wire_pins(bat, 0, r, 0);                     /* kabel 0: sisi (+) -> panas */
    wire_pins(r, 1, gnd, 0);                     /* kabel 1: sisi (-) -> dingin */
    wire_pins(bat, 1, gnd, 0);                   /* kabel 2: (-) -> dingin */
    sim_recompute();
    ASSERT_INT_EQ(3, wire_count);
    ASSERT_INT_EQ(1, sim_wire_hot(0));
    ASSERT_INT_EQ(0, sim_wire_hot(1));
    ASSERT_INT_EQ(0, sim_wire_hot(2));
    ASSERT_INT_EQ(0, sim_wire_hot(-1));        /* guard indeks */
    ASSERT_INT_EQ(0, sim_wire_hot(wire_count));
}

/* ===================== sim_set_running ===================== */

TEST(sim, set_running_mengatur_flag_dan_status) {
    test_reset_model();
    sim_set_running(1);
    ASSERT_INT_EQ(1, sim_running);
    ASSERT_WSTR_HAS(g_status, L"pasang Baterai atau Sumber AC");
    sim_set_running(0);
    ASSERT_INT_EQ(0, sim_running);
    ASSERT_WSTR_HAS(g_status, L"Mode Edit");

    add_at(COMP_BATTERY, 200, 200);
    sim_set_running(1);
    ASSERT_INT_EQ(1, sim_running);
    ASSERT_WSTR_HAS(g_status, L"RUN:");        /* sekarang pakai sim_message() */
    sim_set_running(0);
}

/* ===================== sim_comp_lit guards ===================== */

TEST(sim, sim_comp_lit_guard_null_dan_non_led) {
    test_reset_model();
    Component *bat, *led;
    build_basic_loop(350.0, &bat, &led, NULL);
    sim_recompute();
    ASSERT_INT_EQ(1, sim_comp_lit(led));
    ASSERT_INT_EQ(0, sim_comp_lit(bat));       /* bukan LED slotnya tak pernah lit */
    ASSERT_INT_EQ(0, sim_comp_lit(NULL));
    ASSERT_INT_EQ(0, sim_comp_blown(NULL));
    ASSERT_DBL_NEAR(-1.0, sim_comp_current(NULL), 1e-12);
}

/* ===================== sim_format_si ===================== */

TEST(sim, format_si_presisi_3_angka) {
    wchar_t o[64];
    sim_format_si(0.0318, L"A", o, 64);
    ASSERT_TRUE(wcscmp(o, L"31.8 mA") == 0);
    sim_format_si(220.0, L"\x03A9", o, 64);
    ASSERT_TRUE(wcscmp(o, L"220 \x03A9") == 0);
    sim_format_si(9.0, L"V", o, 64);
    ASSERT_TRUE(wcscmp(o, L"9 V") == 0);
    sim_format_si(1500.0, L"Hz", o, 64);
    ASSERT_TRUE(wcscmp(o, L"1.5 kHz") == 0);
    sim_format_si(0.0, L"F", o, 64);           /* nol masuk cabang pico */
    ASSERT_TRUE(wcscmp(o, L"0 pF") == 0);
    sim_format_si(1e-9, L"F", o, 64);
    ASSERT_TRUE(wcscmp(o, L"1 nF") == 0);
}

/* ===================== Kontrak LDR / Buzzer (implementasi produksi) =======
   Tidak ada test seam lagi: kedua fungsi di bawah diuji LANGSUNG terhadap
   src/sim.c — berkas yang sama yang ditautkan build.bat ke executable
   produksi. Dengan begitu mustahil ada "fungsi tes" yang ikut ter-build. */

TEST(sim, ldr_effective_r_gelap_terang_dan_bukan_ldr) {
    test_reset_model();
    Component *ldr = add_at(COMP_LDR, 200, 200);
    ASSERT_TRUE(ldr->type == COMP_LDR);          /* guard tipe tidak boleh menolak */
    ldr->on = 0;                                 /* GELAP: R = nilai penuh */
    ASSERT_DBL_NEAR(LDR_DARK_DEFAULT, sim_ldr_effective_r(ldr), 1e-6);
    ldr->v = 47e3;                               /* nilai custom dihormati */
    ASSERT_DBL_NEAR(47e3, sim_ldr_effective_r(ldr), 1e-6);
    ldr->on = 1;                                 /* TERANG: R ~ 1% nilai gelap */
    ASSERT_DBL_NEAR(47e3 * LDR_LIGHT_RATIO, sim_ldr_effective_r(ldr), 1e-9);
    /* v <= 0 -> fallback default gelap */
    ldr->v = 0.0; ldr->on = 0;
    ASSERT_DBL_NEAR(LDR_DARK_DEFAULT, sim_ldr_effective_r(ldr), 1e-6);
    /* komponen non-LDR dan NULL -> -1 ("bukan LDR") sesuai kontrak header */
    Component *r = add_at(COMP_RESISTOR, 400, 200);
    ASSERT_DBL_NEAR(-1.0, sim_ldr_effective_r(r), 1e-12);
    ASSERT_DBL_NEAR(-1.0, sim_ldr_effective_r(NULL), 1e-12);
}

TEST(sim, buzzer_on_patuh_guard_tipe_dan_mode) {
    test_reset_model();
    Component *bz = add_at(COMP_BUZZER, 200, 200);
    ASSERT_INT_EQ(0, sim_buzzer_on(bz));         /* mode EDIT -> selalu diam */
    Component *r = add_at(COMP_RESISTOR, 400, 200);
    sim_running = 1;                             /* RUN tapi tanpa rangkaian lengkap:
                                                    buzz_[] tetap 0 utk komponen non-buzzer */
    ASSERT_INT_EQ(0, sim_buzzer_on(r));          /* guard tipe: bukan buzzer -> 0 */
    ASSERT_INT_EQ(0, sim_buzzer_on(NULL));       /* guard NULL -> 0, tidak crash */
    sim_running = 0;
}

/* Entry point suite ini (dipanggil dari test_main.c). */
void suite_sim(void) {
    SUITE_BEGIN("sim.c");
    RUN(sim, canvas_kosong_dan_tanpa_sumber);
    RUN(sim, loop_benar_led_menyala_hukum_ohm);
    RUN(sim, saklar_off_memutus_led_on_menghubungkan);
    RUN(sim, polaritas_terbalik_led_mati);
    RUN(sim, korslet_langsung_plus_ke_minus);
    RUN(sim, korslet_lewat_saklar_on);
    RUN(sim, led_overcurrent_rusak_pelan);
    RUN(sim, led_meledak_tanpa_resistor);
    RUN(sim, led_meledak_arus_di_atas_5x_if_max);
    RUN(sim, vled_ge_vsumber_led_tidak_menyala_terang);
    RUN(sim, dioda_forward_menghantar_reverse_memblokir);
    RUN(sim, dioda_seri_menambah_vdrop);
    RUN(sim, transistor_on_mengalirkan_led);
    RUN(sim, transistor_off_basis_mengambang);
    RUN(sim, kapasitor_memblokir_dc);
    RUN(sim, sumber_ac_menyalakan_led_mode_rms);
    RUN(sim, ac_220v_tanpa_resistor_meledak_dengan_peringatan);
    RUN(sim, kapasitor_tembus_dalam_mode_ac);
    RUN(sim, sim_wire_hot_dan_guard_indeks);
    RUN(sim, set_running_mengatur_flag_dan_status);
    RUN(sim, sim_comp_lit_guard_null_dan_non_led);
    RUN(sim, format_si_presisi_3_angka);
    RUN(sim, ldr_effective_r_gelap_terang_dan_bukan_ldr);
    RUN(sim, buzzer_on_patuh_guard_tipe_dan_mode);
}
