#include "schematic.h"

int sim_running = 0;
int sim_led_count = 0;
int sim_short = 0;
int sim_has_battery = 0;
int sim_has_ac = 0;
int sim_ac_mode = 0;

/* Hasil perhitungan kuantitatif (Ohm + hukum tegangan Kirchhoff) */
double sim_vsrc = 0.0;
double sim_rloop = 0.0;
double sim_iled_max = 0.0;
double sim_vdrop_max = 0.0;   /* total drop Vf pada loop arus terbesar */
int    sim_exploded = 0;
int    sim_overcur = 0;

/* Net listrik: tiap pin komponen = 1 node.
   node = comp_index * 3 + pin (ground pin1/2, basis = pin2 tak dipakai
   komponen 1-2 pin; transistor pakai pin0=C, pin1=E, pin2=B). */
static int parent_[MAX_COMP * 3];
static int lit_[MAX_COMP];
static int cond_[MAX_COMP];
static int dfwd_[MAX_COMP];   /* 1 = dioda menghantar forward (A->K) */
static int buzz_[MAX_COMP];   /* 1 = buzzer ini aktif (berbunyi) saat RUN */
static double vmread_[MAX_COMP]; /* bacaan voltmeter per komponen (V); VM_DISCONNECTED = tak terhubung */
static int    vmconn_[MAX_COMP]; /* 1 = kedua pin voltmeter menyentuh net yang berbeda */

/* Kode "tak terhubung" untuk vmread_[]: salah satu/dua pin mengambang. */
#define VM_DISCONNECTED (-1e30)
static int hot_[MAX_WIRES];
static int blown_[MAX_COMP];  /* 1 = LED meledak (kelebihan arus/tegangan) */
static double ilead_[MAX_COMP]; /* arus hasil hitung per komponen (A); -1 = tak terdefinisi */
static double vdrop_[MAX_COMP]; /* total drop (Vf LED + dioda + Vce) pada loop tiap LED */

/* ---- Utilitas untaian net (BFS pada graf union-find) ---- */
#define NETQ_MAX (MAX_COMP * 3)
static int nrk_[NETQ_MAX];      /* root yang sudah terdaftar dalam queue */
static int qn_[NETQ_MAX];       /* antrean node */

static int net_find_or_add(int root, int *qn, int *nn, int ncap) {
    for (int i = 0; i < *nn; i++) if (nrk_[i] == root) return i;
    if (*nn >= ncap) return -1;
    nrk_[*nn] = root;
    qn[*nn] = root;
    (*nn)++;
    return *nn - 1;
}

static int find_root(int a) {
    while (parent_[a] != a) {
        parent_[a] = parent_[parent_[a]];
        a = parent_[a];
    }
    return a;
}

static void unite(int a, int b) {
    int ra = find_root(a), rb = find_root(b);
    if (ra != rb) parent_[ra] = rb;
}

static int node_of(int comp_idx, int pin) {
    return comp_idx * 3 + pin;
}

/* Cari indeks net ber-root `root` pada daftar hasil BFS (-1 jika tak ada). */
static int net_index_of(int root, int nn) {
    for (int k = 0; k < nn; k++) if (nrk_[k] == root) return k;
    return -1;
}

/* Klasifikasi "jembatan" komponen dua-pin antar net untuk penelusuran
   jalur (net_heads_from) — dihitung SETELAH seluruh union selesai,
   memakai state final pass-1/1b/2:
     BRIDGE_PASSIVE : penghantar ideal yang pin-pinnya SUDAH disatukan
                      union (saklar-ON / dioda-fwd / transistor-ON /
                      kapasitor mode AC murni). Ia tak pernah muncul
                      sebagai jembatan beda-net; bila tetap muncul
                      (anomali), dianggap kawat 0 Ω.
     BRIDGE_RESIST  : resistor/LDR — ikut menghantar tapi menyumbang R;
                      normalnya sudah tergabung union, hanya terlihat
                      sebagai jembatan bila bernilai <= 0.
     BRIDGE_LED     : beban LED (memotong jalur DC; dipakai pass-4).
     BRIDGE_BLOCK   : memutus jalur (kapasitor DC, saklar OFF, dioda
                      reverse, sumber, ground, voltmeter, dll). */
enum { BRIDGE_PASSIVE = 0, BRIDGE_RESIST = 1, BRIDGE_LED = 2, BRIDGE_BLOCK = 3 };

static int bridge_kind_(int i) {
    const Component *c = &comps[i];
    switch (c->type) {
        case COMP_RESISTOR: case COMP_LDR:
            return BRIDGE_RESIST;
        case COMP_BUZZER:   return BRIDGE_PASSIVE;
        case COMP_SWITCH:   return c->on ? BRIDGE_PASSIVE : BRIDGE_BLOCK;
        case COMP_DIODE:    return dfwd_[i] ? BRIDGE_PASSIVE : BRIDGE_BLOCK;
        case COMP_TRANSISTOR: return cond_[i] ? BRIDGE_PASSIVE : BRIDGE_BLOCK;
        case COMP_CAPACITOR: case COMP_CAPOLAR:
            /* catu AC campuran (DC+AC): kapasitor BLOKIR DC (pass-1 tidak
               menggabungnya) -> harus memotong jalur LED juga di pass-3 */
            return sim_ac_mode ? BRIDGE_PASSIVE : BRIDGE_BLOCK;
        case COMP_LED:      return BRIDGE_LED;
        default:            return BRIDGE_BLOCK;   /* sumber/ground/voltmeter */
    }
}

/* Kontribusi satu jembatan penghantar yang masih terlihat sebagai dua net
   terpisah: R ke radd, drop tegangan ke vadd. */
static void bridge_cost(int i, double *radd, double *vadd) {
    const Component *c = &comps[i];
    *radd = 0.0; *vadd = 0.0;
    switch (c->type) {
        case COMP_RESISTOR: *radd = (c->v > 0.0) ? c->v : 0.0; break;
        case COMP_LDR: {
            double reff = sim_ldr_effective_r(c);
            *radd = (reff >= 0.0) ? reff : 0.0;
            break;
        }
        case COMP_DIODE:    *vadd = (c->v > 0.0) ? c->v : VF_DIODE; break;
        case COMP_TRANSISTOR: *vadd = 0.2; break;   /* V_CE saturasi */
        default: break;                             /* kawat ideal 0 V / 0 Ω */
    }
}

/* Kumpulkan daftar root net satu untaian kabel ke dalam qn_/nrk_ lewat BFS. */
static void chain_roots_from(int start, int *nn_out) {
    int nn = 0;
    net_find_or_add(find_root(start), qn_, &nn, NETQ_MAX);
    for (int h = 0; h < nn; h++) {
        int r = qn_[h];
        for (int w = 0; w < wire_count; w++) {
            Component *ca = find_comp_by_id(wires[w].a.comp_id);
            Component *cb = find_comp_by_id(wires[w].b.comp_id);
            if (!ca || !cb) continue;
            int ia = (int)(ca - comps), ib = (int)(cb - comps);
            if (ia < 0 || ia >= comp_count || ib < 0 || ib >= comp_count) continue;
            int ra = find_root(node_of(ia, wires[w].a.pin_idx));
            int rb = find_root(node_of(ib, wires[w].b.pin_idx));
            if (ra == r && rb != r) net_find_or_add(rb, qn_, &nn, NETQ_MAX);
            if (rb == r && ra != r) net_find_or_add(ra, qn_, &nn, NETQ_MAX);
        }
    }
    *nn_out = nn;
}

/* Tabel head[]: untuk tiap net k dalam untaian `roots`, head[k] = komponen
   dua-pin yang menghubungkan net k ke net lain DALAM untaian yang sama
   (jembatan antar-net — mis. kapasitor DC-block, saklar OFF, LED lain).
   Komponen penghantar pasif yang sudah disatukan union tak pernah muncul
   di sini karena kedua pin-nya berada pada root/net yang sama. -1 = buntu. */
static void fill_heads(const int *roots, int nn, int *head, int nhcap) {
    for (int i = 0; i < nhcap && i < nn; i++) head[i] = -1;
    for (int i = 0; i < comp_count; i++) {
        int n = comp_pin_count(&comps[i]);
        if (n < 2) continue;
        int h0 = -1, h1 = -1;
        for (int k = 0; k < nn; k++) {
            if (find_root(node_of(i, 0)) == roots[k]) h0 = k;
            if (find_root(node_of(i, 1)) == roots[k]) h1 = k;
        }
        if (h0 >= 0 && h1 >= 0 && h0 != h1) { head[h0] = i; head[h1] = i; }
    }
}

/* Biaya resistif antara dua SIMPUL graf mentah satu untaian:
   - kedua simpul satu ROOT (mis. dua ujung resistor yang sudah disatukan
     union pass-1): resistor/LDR internal menyumbang R-nya; penghantar
     lain (kawat/saklar/dioda-fwd/transistor-ON/kapasitor-AC) ideal 0 Ω.
   - beda root (jembatan anomali yang tak tergabung, mis. resistor 0 Ω):
     pakai bridge_cost komponen yang menyentuh kedua node tsb.
   Mengembalikan -1 bila pasangan simpul tidak terhubung penghantar mana pun. */
static double edge_cost_(int cs, int nb, const int *gmap_rev) {
    int nds = gmap_rev[cs], ndt = gmap_rev[nb];
    int rs = find_root(nds), rt = find_root(ndt);
    if (rs == rt) {
        /* cari komponen penghantar yang kedua pin-nya = nds & ndt (beda node) */
        for (int i = 0; i < comp_count; i++) {
            if (comp_pin_count(&comps[i]) < 2) continue;
            int bk = bridge_kind_(i);
            if (bk == BRIDGE_BLOCK || bk == BRIDGE_LED) continue;
            int p0 = node_of(i, 0), p1 = node_of(i, 1);
            if ((p0 == nds && p1 == ndt) || (p1 == nds && p0 == ndt)) {
                if (comps[i].type == COMP_RESISTOR)
                    return (comps[i].v > 0.0) ? comps[i].v : 0.0;
                if (comps[i].type == COMP_LDR) {
                    double reff = sim_ldr_effective_r(&comps[i]);
                    return (reff >= 0.0) ? reff : 0.0;
                }
                return 0.0;
            }
        }
        return 0.0;   /* murni kabel */
    }
    return -1.0;      /* beda root: hanya lewat sisi jembatan eksplisit */
}

/* Bangun graf "net mentah" pada SATU untaian kabel: simpul = node pin
   (comp*3+pin) yang root-nya termasuk untaian `roots`; sisi = kabel +
   jembatan penghantar pasif final. Mengembalikan jumlah simpul;
   `map[node]` = indeks simpul (-1 bila bukan bagian untaian);
   `rev[idx]` = node asal tiap simpul; `edges` = matriks bool ns*ns. */
#define NG_MAX (MAX_COMP * 3)
static int raw_graph_build(const int *roots, int nn, int *map, int *rev, char *edges) {
    int ns = 0;
    for (int i = 0; i < NG_MAX; i++) map[i] = -1;
    for (int i = 0; i < comp_count; i++) {
        int npins = comp_pin_count(&comps[i]);
        for (int p = 0; p < npins; p++) {
            int nd = node_of(i, p);
            if (map[nd] >= 0) continue;
            int r = find_root(nd);
            int hit = -1;
            for (int k = 0; k < nn; k++) if (roots[k] == r) { hit = k; break; }
            if (hit >= 0 && ns < NG_MAX) { map[nd] = ns; rev[ns] = nd; ns++; }
        }
    }
    for (int a = 0; a < ns; a++)
        for (int b = 0; b < ns; b++) edges[a * ns + b] = 0;
    for (int w = 0; w < wire_count; w++) {
        Component *ca = find_comp_by_id(wires[w].a.comp_id);
        Component *cb = find_comp_by_id(wires[w].b.comp_id);
        if (!ca || !cb) continue;
        int ia = (int)(ca - comps), ib = (int)(cb - comps);
        if (ia < 0 || ia >= comp_count || ib < 0 || ib >= comp_count) continue;
        int na = node_of(ia, wires[w].a.pin_idx);
        int nb = node_of(ib, wires[w].b.pin_idx);
        if (map[na] >= 0 && map[nb] >= 0) {
            edges[map[na] * ns + map[nb]] = 1;
            edges[map[nb] * ns + map[na]] = 1;
        }
    }
    /* jembatan penghantar final (resistor/LDR/saklar-ON/buzzer/dioda-fwd/
       transistor-ON/kapasitor AC): kedua pin boleh beda-node walau satu
       root (hasil union) — tambahkan sebagai sisi agar penelusuran jalur
       tetap melihat R per komponen. */
    for (int i = 0; i < comp_count; i++) {
        if (comp_pin_count(&comps[i]) < 2) continue;
        int bk = bridge_kind_(i);
        if (bk == BRIDGE_BLOCK || bk == BRIDGE_LED) continue;
        int n0 = node_of(i, 0), n1 = node_of(i, 1);
        if (n0 >= 0 && n0 < NG_MAX && n1 >= 0 && n1 < NG_MAX &&
            map[n0] >= 0 && map[n1] >= 0) {
            edges[map[n0] * ns + map[n1]] = 1;
            edges[map[n1] * ns + map[n0]] = 1;
        }
    }
    return ns;
}

/* BFS dari node `start` mengikuti kabel saja (union tanpa komponen).
   Mengisi nrk_/qn_ (daftar net satu untaian, max *nn_out), dan head[] =
   index komponen dua-pin yang menyentuh tiap net (-1 = buntu).
   Jembatan penghantar pasif (sudah disatukan union) dilewati secara
   transparan: bila sebuah komponen passive menyentuh suatu net lewat
   kedua pin-nya yang kini satu root, komponen itu TIDAK dianggap
   jembatan — ia sudah menjadi bagian dari net itu sendiri. Hanya
   komponen yang pin-nya benar-benar beda-net yang tercatat sebagai head
   (LED / kapasitor DC / saklar OFF / dioda reverse / sumber / ground,
   atau passive anomali mis. resistor 0 Ω yang tak tergabung). */
static void net_heads_from(int start, int *head, int nhcap, int *nn_out) {
    for (int i = 0; i < nhcap; i++) head[i] = -1;
    int nn = 0;
    chain_roots_from(start, &nn);
    /* nrk_/qn_ telah terisi oleh chain_roots_from (nrk_ mirror qn_) */
    fill_heads(nrk_, nn, head, nhcap);
    *nn_out = nn;
}

void sim_recompute(void) {
    for (int i = 0; i < MAX_COMP * 3; i++) parent_[i] = i;
    for (int i = 0; i < MAX_COMP; i++) {
        lit_[i] = 0; cond_[i] = 0; dfwd_[i] = 0; buzz_[i] = 0;
        blown_[i] = 0; ilead_[i] = -1.0; vdrop_[i] = 0.0;
        vmread_[i] = VM_DISCONNECTED; vmconn_[i] = 0;
    }
    for (int i = 0; i < MAX_WIRES; i++) hot_[i] = 0;
    sim_led_count = 0;
    sim_short = 0;
    sim_has_battery = 0;
    sim_has_ac = 0;
    sim_ac_mode = 0;
    sim_vsrc = 0.0;
    sim_rloop = 0.0;
    sim_iled_max = 0.0;
    sim_vdrop_max = 0.0;
    sim_exploded = 0;
    sim_overcur = 0;

    for (int i = 0; i < wire_count; i++) {
        Component *ca = find_comp_by_id(wires[i].a.comp_id);
        Component *cb = find_comp_by_id(wires[i].b.comp_id);
        if (!ca || !cb) continue;
        int ia = (int)(ca - comps), ib = (int)(cb - comps);
        if (ia < 0 || ia >= comp_count || ib < 0 || ib >= comp_count) continue;
        unite(node_of(ia, wires[i].a.pin_idx), node_of(ib, wires[i].b.pin_idx));
    }

    /* Pass 1: penghantar pasif. Resistor & saklar-ON tembus.
       Kapasitor (biasa & polar) = blokir DC (tidak digabung), kecuali
       mode murni-AC (ada sumber AC, tanpa baterai): kapasitor dianggap
       menghantar (impedansi AC) agar loop AC bisa dianalisis.
       Dioda ditunda (butuh arah tegangan), transistor ditunda. */
    int ac_only = 0;
    {
        int has_ac = 0, has_dc = 0;
        for (int i = 0; i < comp_count; i++) {
            if (comps[i].type == COMP_ACSOURCE) has_ac = 1;
            if (comps[i].type == COMP_BATTERY)  has_dc = 1;
        }
        ac_only = (has_ac && !has_dc);
    }
    for (int i = 0; i < comp_count; i++) {
        if (comps[i].type == COMP_RESISTOR) {
            unite(node_of(i, 0), node_of(i, 1));
        } else if (comps[i].type == COMP_LDR) {
            /* LDR = resistor variabel: selalu menghantar (nilai ikut cahaya) */
            unite(node_of(i, 0), node_of(i, 1));
        } else if (comps[i].type == COMP_BUZZER) {
            /* Buzzer ideal: impedansi rendah -> tembus untuk analisis net */
            unite(node_of(i, 0), node_of(i, 1));
        } else if (comps[i].type == COMP_SWITCH && comps[i].on) {
            unite(node_of(i, 0), node_of(i, 1));
        } else if (ac_only &&
                   (comps[i].type == COMP_CAPACITOR || comps[i].type == COMP_CAPOLAR)) {
            unite(node_of(i, 0), node_of(i, 1));
        }
        if (comps[i].type == COMP_BATTERY) sim_has_battery = 1;
        if (comps[i].type == COMP_ACSOURCE) sim_has_ac = 1;
    }
    /* Mode AC: tak ada baterai DC tapi ada sumber AC -> pakai V RMS AC. */
    sim_ac_mode = (!sim_has_battery && sim_has_ac);

    int plus_root[MAX_COMP * 3], minus_root[MAX_COMP * 3];
    int n_plus = 0, n_minus = 0;
    for (int i = 0; i < comp_count; i++) {
        if (comps[i].type == COMP_BATTERY) {
            plus_root[n_plus++] = find_root(node_of(i, 0));
            minus_root[n_minus++] = find_root(node_of(i, 1));
        } else if (comps[i].type == COMP_ACSOURCE) {
            /* Sumber AC: pin0 = Live (+ sesaat), pin1 = Netral (-/ground). */
            plus_root[n_plus++] = find_root(node_of(i, 0));
            minus_root[n_minus++] = find_root(node_of(i, 1));
        } else if (comps[i].type == COMP_GROUND) {
            minus_root[n_minus++] = find_root(node_of(i, 0));
        }
    }

    for (int a = 0; a < n_plus; a++)
        for (int b = 0; b < n_minus; b++)
            if (plus_root[a] == minus_root[b]) sim_short = 1;

    /* Pass 1b: dioda ideal — menghantar satu arah A->K jika anoda(pin0)
       terhubung ke net (+). Keputusan diambil dari net pass-1,
       baru sesudahnya pin A & K digabung (stabil, tanpa urutan sensitif). */
    {
        int di_on[MAX_COMP] = { 0 };
        for (int i = 0; i < comp_count; i++) {
            if (comps[i].type != COMP_DIODE) continue;
            int ra = find_root(node_of(i, 0));
            for (int k = 0; k < n_plus; k++)
                if (plus_root[k] == ra) { di_on[i] = 1; break; }
        }
        for (int i = 0; i < comp_count; i++)
            if (di_on[i]) { dfwd_[i] = 1; unite(node_of(i, 0), node_of(i, 1)); }
        /* root bisa bergeser setelah union A-K -> segarkan acuan (+)/(-)
           SEBELUM pass-2 & evaluasi LED (sama seperti pada transistor).
           Catatan: sim_short sengaja TIDAK dihitung ulang — dioda ideal
           yang menghubungkan (+) ke (-) tetap dilaporkan sebagai korslet. */
        {
            int kp = 0, km = 0;
            for (int i = 0; i < comp_count; i++) {
                if (comps[i].type == COMP_BATTERY ||
                    comps[i].type == COMP_ACSOURCE) {
                    plus_root[kp++]  = find_root(node_of(i, 0));
                    minus_root[km++] = find_root(node_of(i, 1));
                } else if (comps[i].type == COMP_GROUND) {
                    minus_root[km++] = find_root(node_of(i, 0));
                }
            }
        }
    }

    /* Pass 2: transistor NPN — basis kena (+) dan emitor kena (-)
       => C-E tembus (gabungkan pin0 & pin1). Cek dulu, gabung sesudahnya
       supaya keputusan tiap transistor pakai net pass-1 (stabil). */
    {
        int tr_on[MAX_COMP] = { 0 };
        for (int i = 0; i < comp_count; i++) {
            if (comps[i].type != COMP_TRANSISTOR) continue;
            int rb = find_root(node_of(i, 2));
            int re = find_root(node_of(i, 1));
            int b_plus = 0, e_minus = 0;
            for (int k = 0; k < n_plus; k++) if (plus_root[k] == rb) { b_plus = 1; break; }
            for (int k = 0; k < n_minus; k++) if (minus_root[k] == re) { e_minus = 1; break; }
            if (b_plus && e_minus) tr_on[i] = 1;
        }
        for (int i = 0; i < comp_count; i++) {
            if (tr_on[i]) { cond_[i] = 1; unite(node_of(i, 0), node_of(i, 1)); }
        }
        /* root bisa bergeser setelah union C-E -> segarkan acuan (+)/(-) */
        {
            int kp = 0, km = 0;
            for (int i = 0; i < comp_count; i++) {
                if (comps[i].type == COMP_BATTERY ||
                    comps[i].type == COMP_ACSOURCE) {
                    plus_root[kp++] = find_root(node_of(i, 0));
                }
            }
            for (int i = 0; i < comp_count; i++) {
                if (comps[i].type == COMP_BATTERY ||
                    comps[i].type == COMP_ACSOURCE) {
                    minus_root[km++] = find_root(node_of(i, 1));
                } else if (comps[i].type == COMP_GROUND) {
                    minus_root[km++] = find_root(node_of(i, 0));
                }
            }
        }
    }


    /* ---- Pass 3: PENYALAAN LED + PERHITUNGAN NILAI (Hukum Ohm + KVL).
       Model listrik per LOOP LED (bukan aturan global seluruh canvas):
         1. Telusuri SEMUA jalur konduktif dari anoda LED kembali ke kutub
            (+) sumber pada untaian netnya (BFS di graf net; jembatan antar
            net = komponen dua-pin penghantar: R/LDR/saklar-ON/buzzer/
            dioda-fwd/transistor-ON/kapasitor mode AC — nilainya dihitung
            ulang via bridge_cost() karena union pass-1 sudah menyatukan
            pin-pinnya sehingga tidak terlihat sebagai jembatan lagi).
            Tidak ada jalur kembali ke (+) => loop TERBUKA: arus tak
            terdefinisi, LED TIDAK meledak dan TIDAK dikorsletkan.
            (Kesalahan lama: jalur cacat diklasifikasi "tak hingga" dan
            nilai tanpa-R dianggap ledak meski loop belum tertutup —
            rangkaian yang sebenarnya aman tetap dituduh MELEDAK.)
         2. Hukum Ohm + KVL tiap jalur: I = (V_loop - total_drop)/R_jalur
            dengan V_loop = tegangan sumber pada untaian anoda LED itu
            sendiri (sumber seri dijumlahkan; loop berbeda dinilai sendiri,
            bukan maksimum global — regresi AC-terpisah 220 V).
         3. Percabangan paralel: dipakai jalur dengan resistansi TERKECIL
            (konduktansi terbesar -> arus terbesar -> konservatif).
         4. Nasib LED murni hasil perhitungan terhadap datasheet:
              - loop tertutup TANPA resistansi (R = 0) -> I ~ tak hingga
                -> MELEDAK (arus pendek nyata, mis. baterai -> kawat -> LED)
              - I > 5x If_max (100 mA)                         -> MELEDAK
              - I > If_max DAN sumber loop > rating bolak-balik
                maksimum LED (V_EXPLODE = 50 V)               -> MELEDAK
              - If_max < I <= 5x If_max                        -> overcurrent
            Ambang memakai toleransi relatif 1e-6 supaya nilai IDEAL
            (9-2)V/350Ω = 20.000 mA selalu dinyatakan AMAN. ---- */
    {
        static int head_[NETQ_MAX];
        static double jr_[NETQ_MAX], jd_[NETQ_MAX];
        static int q_[NETQ_MAX], seen_[NETQ_MAX];

        for (int i = 0; i < comp_count; i++) {
            if (comps[i].type != COMP_LED) continue;
            int r0 = find_root(node_of(i, 0));   /* anoda */
            int r1 = find_root(node_of(i, 1));   /* katoda */
            int on_plus = 0, on_minus = 0;
            for (int k = 0; k < n_plus; k++) if (plus_root[k] == r0) on_plus = 1;
            for (int k = 0; k < n_minus; k++) if (minus_root[k] == r1) on_minus = 1;
            if (!(on_plus && on_minus)) continue;
            lit_[i] = 1;
            sim_led_count++;

            /* Buzzer yang segaris ikut aktif (berbunyi) bila loop LED ini hidup.
               (Pendekatan visual: buzzer seri/paralel pada loop bertegangan.) */
            for (int b = 0; b < comp_count; b++) {
                if (comps[b].type != COMP_BUZZER) continue;
                int rb0 = find_root(node_of(b, 0));
                int rb1 = find_root(node_of(b, 1));
                int bp = 0, bm = 0;
                for (int k = 0; k < n_plus; k++) {
                    if (plus_root[k] == rb0) bp = 1;
                    if (plus_root[k] == rb1) bp = 1;
                }
                for (int k = 0; k < n_minus; k++) {
                    if (minus_root[k] == rb0) bm = 1;
                    if (minus_root[k] == rb1) bm = 1;
                }
                if ((bp && bm) || (rb0 == r0 || rb1 == r0 || rb0 == r1 || rb1 == r1))
                    buzz_[b] = 1;
            }

            /* (1) untaian net dari anoda mengikuti kabel saja */
            int nn = 0;
            net_heads_from(node_of(i, 0), head_, NETQ_MAX, &nn);
            int sn = net_index_of(r0, nn);
            if (sn < 0) { ilead_[i] = -1.0; continue; }

            /* (2) V_loop: jumlah tegangan semua sumber whose (+) duduk pada
                   untaian anoda ini (seri terakumulasi). Fallback terukur:
                   bila (+) tak berada di untaian ini (mis. tersambung ke
                   anoda lewat dioda/transistor), pakai baterai terbesar,
                   lalu AC terbesar. */
            double vloop = 0.0;
            for (int s = 0; s < comp_count; s++) {
                if (comps[s].type != COMP_BATTERY && comps[s].type != COMP_ACSOURCE)
                    continue;
                int rp = find_root(node_of(s, 0));
                int hit = 0;
                for (int k = 0; k < nn; k++) if (nrk_[k] == rp) { hit = 1; break; }
                if (!hit) continue;
                double vv = comps[s].v;
                if (vv <= 0.0) vv = (comps[s].type == COMP_ACSOURCE) ? 220.0 : 9.0;
                vloop += vv;                       /* sumber seri dijumlahkan */
            }
            if (vloop <= 0.0) {
                double vb = 0.0, va = 0.0;
                for (int s = 0; s < comp_count; s++) {
                    double vv = comps[s].v;
                    if (comps[s].type == COMP_BATTERY) {
                        if (vv <= 0.0) vv = 9.0;
                        if (vv > vb) vb = vv;
                    } else if (comps[s].type == COMP_ACSOURCE) {
                        if (vv <= 0.0) vv = 220.0;
                        if (vv > va) va = vv;
                    }
                }
                vloop = (vb > 0.0) ? vb : va;
            }

            /* (3) Semua jalur konduktif dari anoda kembali ke kutub (+),
               ditelusuri pada GRAF NET MENTAH untaian kabel ini (simpul =
               node pin, sisi = kabel + jembatan penghantar). Union pass-1
               sudah menyatukan penghantar pasif sehingga penelusuran di
               level root kehilangan jejak R per komponen — graf mentah
               memulihkannya. Biaya per langkah antar simpul:
                 - kedua simpul satu ROOT  : intra_net_r(root) (R internal,
                   mis. resistor yang pin-pinnya tergabung; kawat = 0 Ω)
                 - beda root (jembatan anomali, mis. resistor 0 Ω / kapasitor
                   AC yang tak tergabung): bridge_cost()
               Target: simpul dengan root == root kutub (+) sumber mana pun
               -> loop tertutup. Tidak ada target terjangkau => loop TERBUKA
               (arus tak terdefinisi, BUKAN korslet/meledak). */
            double vf_led = (comps[i].v > 0.0 ? comps[i].v : VF_LED_DEFAULT);
            static int gmap_[NG_MAX];
            static char gedges_[NG_MAX * NG_MAX];   /* 768^2 byte ~ 0.6 MB */
            int ns = raw_graph_build(nrk_, nn, gmap_, gedges_);
            int a_node = node_of(i, 0);             /* anoda LED */
            double best_r = -1.0;      /* jalur terpilih: R terkecil */
            double best_drop = 0.0;
            if (ns > 0 && gmap_[a_node] >= 0) {
                static double gr_[NG_MAX];
                static int gseen_[NG_MAX];
                int qh = 0, qt = 0, guard = 0;
                for (int s2 = 0; s2 < ns; s2++) { gseen_[s2] = 0; gr_[s2] = 0.0; }
                int start = gmap_[a_node];
                gseen_[start] = 1; gr_[start] = 0.0; q_[qt++] = start;
                /* antrean pakai q_ (ukuran NETQ_MAX cukup? simpul bisa >NETQ_MAX
                   bila banyak pin; pakai antrean lokal berukuran NS) */
                static int gnq_[NG_MAX];
                qh = 0; qt = 0;
                gnq_[qt++] = start;
                while (qh < qt && guard++ <= 4 * ns + 8) {
                    int cs = gnq_[qh++];
                    int croot = find_root(node_pin_of_(cs)); /* root simpul cs */
                    /* jangan menembus sumber: simpul yang root-nya = (+) atau (-)
                       boleh DITETAPKAN sebagai akhir jalur, tapi tidak dilanjutkan */
                    int is_plus = 0;
                    for (int k = 0; k < n_plus; k++)
                        if (plus_root[k] == croot) { is_plus = 1; break; }
                    if (is_plus && cs != start) {
                        /* kembali ke (+): loop tertutup */
                        if (best_r < 0.0 || gr_[cs] < best_r) {
                            best_r = gr_[cs]; best_drop = vf_led;
                        }
                        continue;
                    }
                    for (int nb = 0; nb < ns; nb++) {
                        if (!gedges_[cs * ns + nb]) continue;
                        int nroot = find_root(node_pin_of_(nb));
                        double addR = 0.0, addV = 0.0;
                        if (nroot == croot) {
                            addR = intra_net_r(croot);
                        } else {
                            /* cari komponen jembatan penghantar antara dua node */
                            int found = 0;
                            for (int cc = 0; cc < comp_count; cc++) {
                                if (comp_pin_count(&comps[cc]) < 2) continue;
                                int bk = bridge_kind_(cc);
                                if (bk == BRIDGE_BLOCK || bk == BRIDGE_LED) continue;
                                int p0 = node_of(cc, 0), p1 = node_of(cc, 1);
                                int m0 = (p0 < NG_MAX) ? gmap_[p0] : -1;
                                int m1 = (p1 < NG_MAX) ? gmap_[p1] : -1;
                                if ((m0 == cs && m1 == nb) || (m1 == cs && m0 == nb)) {
                                    bridge_cost(cc, &addR, &addV);
                                    found = 1; break;
                                }
                            }
                            if (!found) continue;    /* bukan penghantar -> buntu */
                            if (addV > 0.0) best_drop_extra_ += addV; /* lihat catatan */
                        }
                        double nr = gr_[cs] + addR;
                        if (!gseen_[nb]) {
                            gseen_[nb] = 1; gr_[nb] = nr; gnq_[qt++] = nb;
                        } else if (nr < gr_[nb]) {
                            gr_[nb] = nr; gnq_[qt++] = nb;   /* BF ringan */
                        }
                    }
                }
            }
            /* Anoda duduk di net (+) DAN katoda duduk di net (-): loop hanya
               terdiri kabel + LED saja (semua penghantar lain sudah tergabung
               ke dalam satu root yang sama dengan (+)/(-)). Bila BFS di atas
               tidak menemukan jalur kembali, loop ini tetap tertutup via
               sumber -> arus pendek tanpa resistansi (MELEDAK).
               Pengecualian: baterai "terpotong" oleh kapasitor DC-block —
               root (+)-nya adalah hasil union yang menyilang kapasitor, jadi
               jalur 0 Ω itu tidak nyata. Deteksi: ada kapasitor blokir yang
               salah satu pin-nya ikut root (+)/(-) LED ini. */
            {
                int dc_block_split = 0;
                if (!sim_ac_mode) {
                    for (int c2 = 0; c2 < comp_count && !dc_block_split; c2++) {
                        if (comps[c2].type != COMP_CAPACITOR &&
                            comps[c2].type != COMP_CAPOLAR) continue;
                        if (bridge_kind_(c2) != BRIDGE_BLOCK) continue;
                        int rc0 = find_root(node_of(c2, 0));
                        int rc1 = find_root(node_of(c2, 1));
                        int touch_p = 0, touch_m = 0;
                        for (int k = 0; k < n_plus; k++) {
                            if (plus_root[k] == rc0 || plus_root[k] == rc1) touch_p = 1;
                        }
                        for (int k = 0; k < n_minus; k++) {
                            if (minus_root[k] == rc0 || minus_root[k] == rc1) touch_m = 1;
                        }
                        /* kapasitor memotong untaian anoda LED? */
                        int on_anode_chain = 0;
                        for (int k = 0; k < nn; k++)
                            if (nrk_[k] == rc0 || nrk_[k] == rc1) { on_anode_chain = 1; break; }
                        if (touch_p && (on_anode_chain || touch_m)) dc_block_split = 1;
                    }
                }
                if (!dc_block_split) {
                    int anode_on_plus = 0;
                    for (int k = 0; k < n_plus; k++) if (plus_root[k] == r0) anode_on_plus = 1;
                    if (anode_on_plus && on_minus) {
                        /* jalur 0 Ω: R loop nol -> arus pendek */
                        if (best_r < 0.0) { best_r = 0.0; best_drop = vf_led; }
                    }
                }
            }

            /* (4) arus & nasib LED */
            double iled;
            if (best_r < 0.0) {
                /* Loop terbuka: tidak ada arus terdefinisi. LED dicatat
                   terhubung (lit_ topologi) tapi BUKAN korslet/meledak. */
                ilead_[i] = -1.0;
                continue;
            }
            if (best_r <= 0.0) {
                iled = 1e9;                              /* tanpa R: ~tak hingga */
            } else {
                double dv = vloop - best_drop;
                iled = (dv > 0.0) ? dv / best_r : 0.0;   /* Vf >= Vbat -> tak menyala */
            }
            ilead_[i] = iled;
            vdrop_[i] = best_drop;
            if (iled > sim_iled_max) {
                sim_iled_max = iled; sim_rloop = best_r;
                sim_vsrc = vloop; sim_vdrop_max = best_drop;
            } else if (sim_iled_max >= 1e8 && iled < 1e8) {
                /* ada LED korslet (arus "tak hingga") — tetap tampilkan
                   angka loop LED yang benar-benar tersambung */
                sim_iled_max = iled; sim_rloop = best_r;
                sim_vsrc = vloop; sim_vdrop_max = best_drop;
            }

            const double tol = 1.0 + 1e-6;
            if (iled >= 1e8 || iled > 5.0 * VF_LED_MAX * tol ||
                (iled > VF_LED_MAX * tol && vloop > V_EXPLODE)) {
                blown_[i] = 1;
                sim_exploded = 1;
            } else if (iled > VF_LED_MAX * tol) {
                blown_[i] = 1;
                sim_overcur = 1;
            }
        }
    }

    /* ---- Pass 4: VOLTMETER — ukur BEDA POTENSIAL (tegangan) antara dua titik.
       Voltmeter ideal = impedansi tak hingga: pin0 & pin1 TIDAK PERNAH
       digabung menjadi satu net, jadi memasangnya tidak mengubah rangkaian.
       Peta potensial per net (volt terhadap kutub (-) sumber):
         - mulai dari tiap net 0 V (kutub "-" baterai / netral AC / ground),
           telusuri rantai dua-pin sepanjang kabel: penghantar pasif
           (R/LDR/saklar-ON/buzzer/dioda fwd/transistor on/kapasitor mode AC)
           -> potensial tetap; beban aktif (LED/dioda) -> turun sebesar Vf.
         - net yang memuat (+) baterai atau Live AC -> +V untaian sumber
           (baterai seri dijumlahkan).
       Bacaan tiap voltmeter = potensial(pin0) - potensial(pin1);
       terbalik kutub -> negatif; salah satu pin mengambang -> VM_DISCONNECTED. */
    {
        static double vm_net[MAX_COMP * 3];   /* potensial per root net (V) */
        static int    vm_has[MAX_COMP * 3];   /* 1 = net punya info potensial */
        for (int k = 0; k < MAX_COMP * 3; k++) { vm_net[k] = 0.0; vm_has[k] = 0; }

        /* (a) tandai net referensi: minus_ref (kandidat 0 V), plus_ref
             (kandidat +V untaian sumber). */
        int minus_ref[MAX_COMP * 3], plus_ref[MAX_COMP * 3];
        int n_mref = 0, n_pref = 0;
        for (int i = 0; i < comp_count; i++) {
            if (comps[i].type == COMP_BATTERY || comps[i].type == COMP_ACSOURCE) {
                int rm = find_root(node_of(i, 1));
                int rp = find_root(node_of(i, 0));
                int dupm = 0, dupp = 0;
                for (int k = 0; k < n_mref; k++) if (minus_ref[k] == rm) dupm = 1;
                for (int k = 0; k < n_pref; k++) if (plus_ref[k] == rp) dupp = 1;
                if (!dupm && n_mref < MAX_COMP * 3) minus_ref[n_mref++] = rm;
                if (!dupp && n_pref < MAX_COMP * 3) plus_ref[n_pref++] = rp;
            } else if (comps[i].type == COMP_GROUND) {
                int rg = find_root(node_of(i, 0));
                int dup = 0;
                for (int k = 0; k < n_mref; k++) if (minus_ref[k] == rg) dup = 1;
                if (!dup && n_mref < MAX_COMP * 3) minus_ref[n_mref++] = rg;
            }
        }

        /* (b) potensial tiap net "+": jumlah tegangan sumber yang tergabung
             seri (kutub "-" suatu sumber duduk di net "+" sumber lain).
             Hanya net "+" yang TIDAK terpetakan oleh BFS 0-V yang memakai
             nilai ini — sehingga pembagi tegang tetap muncul alami. */
        double vstrand[MAX_COMP * 3];   /* paralel dgn plus_ref[] */
        for (int k = 0; k < n_pref; k++) {
            double mag = 0.0;
            int rp = plus_ref[k];
            int used[MAX_COMP];
            for (int i = 0; i < comp_count; i++) used[i] = 0;
            for (int i = 0; i < comp_count; i++) {
                if (comps[i].type != COMP_BATTERY && comps[i].type != COMP_ACSOURCE)
                    continue;
                if (find_root(node_of(i, 0)) != rp) continue;
                double acc = 0.0;
                int cur = i, guard = 0;
                while (cur >= 0 && guard++ <= comp_count) {
                    if (used[cur]) break;
                    used[cur] = 1;
                    double vv = comps[cur].v;
                    if (comps[cur].type == COMP_ACSOURCE) { if (vv <= 0.0) vv = 220.0; }
                    else                                  { if (vv <= 0.0) vv = 9.0;  }
                    acc += vv;
                    int rm = find_root(node_of(cur, 1));
                    int nxt = -1;
                    for (int j = 0; j < comp_count; j++) {
                        if (j == cur || used[j]) continue;
                        if (comps[j].type != COMP_BATTERY && comps[j].type != COMP_ACSOURCE)
                            continue;
                        if (find_root(node_of(j, 0)) == rm) { nxt = j; break; }
                    }
                    cur = nxt;
                }
                if (acc > mag) mag = acc;   /* beberapa jalur -> pakai terbesar */
            }
            vstrand[k] = mag;
        }

        /* (c) BFS "rantai head" dari tiap net 0 V: head[] = komponen dua-pin
             yang menyentuh net (jembatan antar net dalam satu untaian kabel).
             Penghantar pasif dilewati tanpa drop; LED/dioda menurunkan
             potensial sebesar Vf; kapasitor blokir DC (hanya dilalui saat
             mode AC murni); sumber memutus rantai. Net yang sudah punya
             potensial tidak ditimpa (first-wins). */
        static int hv_head[NETQ_MAX];
        static int hv_q[NETQ_MAX];
        static double hv_pot[NETQ_MAX];
        for (int m = 0; m < n_mref; m++) {
            int start = minus_ref[m];
            int qh = 0, qt = 0;
            int tmp_nn = 0;
            net_heads_from(start * 3, hv_head, NETQ_MAX, &tmp_nn);
            int si = net_index_of(find_root(start), tmp_nn);
            if (si < 0) continue;
            hv_q[qt++] = si;
            hv_pot[si] = 0.0;
            while (qh < qt) {
                int ci = hv_q[qh++];
                int h = hv_head[ci];
                if (h < 0) continue;                 /* buntu */
                Component *hc = &comps[h];
                int other_pin = (find_root(node_of(h, 0)) == nrk_[ci]) ? 1 : 0;
                int oroot = find_root(node_of(h, other_pin));
                int oi = net_index_of(oroot, tmp_nn);
                if (oi < 0 || oi < qh - 1) continue; /* sudah/anomali */
                double npot = hv_pot[ci];
                int passable = 0;
                switch (hc->type) {
                    case COMP_RESISTOR: case COMP_LDR: case COMP_BUZZER:
                        passable = 1; break;
                    case COMP_SWITCH:   passable = hc->on; break;
                    case COMP_DIODE:    passable = dfwd_[h]; break;
                    case COMP_TRANSISTOR: passable = cond_[h]; break;
                    case COMP_CAPACITOR: case COMP_CAPOLAR:
                        passable = sim_ac_mode; break;
                    default: break;
                }
                if (passable) {
                    hv_pot[oi] = npot;
                } else if (hc->type == COMP_LED) {
                    double vf = (hc->v > 0.0) ? hc->v : VF_LED_DEFAULT;
                    hv_pot[oi] = npot - vf;          /* drop maju LED */
                } else if (hc->type == COMP_DIODE) {
                    double vf = (hc->v > 0.0) ? hc->v : VF_DIODE;
                    hv_pot[oi] = npot - vf;          /* drop mundur (pendekatan) */
                } else {
                    continue;                        /* baterai/ground -> stop */
                }
                hv_q[qt++] = oi;
            }
            for (int j = 0; j < tmp_nn; j++) {
                int r = nrk_[j];
                if (r >= 0 && r < MAX_COMP * 3 && !vm_has[r]) {
                    vm_net[r] = hv_pot[j]; vm_has[r] = 1;
                }
            }
        }

        /* (d) sisa net "+" yang belum terpetakan (mis. langsung tersambung
             ke kutub + lewat kabel) -> potensial = +V untaian sumbernya. */
        for (int k = 0; k < n_pref; k++) {
            int r = plus_ref[k];
            if (r >= 0 && r < MAX_COMP * 3 && !vm_has[r]) {
                vm_net[r] = vstrand[k]; vm_has[r] = 1;
            }
        }

        /* (e) bacaan tiap voltmeter = potensial(pin0) - potensial(pin1). */
        for (int i = 0; i < comp_count; i++) {
            if (comps[i].type != COMP_VOLTMETER) continue;
            int r0 = find_root(node_of(i, 0));
            int r1 = find_root(node_of(i, 1));
            if (!vm_has[r0] || !vm_has[r1]) continue;   /* salah satu mengambang */
            vmconn_[i] = 1;
            vmread_[i] = vm_net[r0] - vm_net[r1];
        }
    }

    /* Hot-wire: kabel yang ujung-a-nya duduk di net kutub (+) dianggap
       bertegangan (digambar merah/panas saat RUN). */
    for (int i = 0; i < wire_count; i++) {
        Component *ca = find_comp_by_id(wires[i].a.comp_id);
        if (!ca) continue;
        int ia = (int)(ca - comps);
        if (ia < 0 || ia >= comp_count) continue;
        int r = find_root(node_of(ia, wires[i].a.pin_idx));
        for (int k = 0; k < n_plus; k++)
            if (plus_root[k] == r) { hot_[i] = 1; break; }
    }

    /* Buzzer mandiri: berbunyi bila pin0 di net (+) dan pin1 di net (-),
       walau tidak ada LED pada loop (mis. AC -> buzzer langsung). */
    for (int i = 0; i < comp_count; i++) {
        if (comps[i].type != COMP_BUZZER || buzz_[i]) continue;
        int r0 = find_root(node_of(i, 0));
        int r1 = find_root(node_of(i, 1));
        int on_plus = 0, on_minus = 0;
        for (int k = 0; k < n_plus; k++) if (plus_root[k] == r0) on_plus = 1;
        for (int k = 0; k < n_minus; k++) if (minus_root[k] == r1) on_minus = 1;
        if (on_plus && on_minus) buzz_[i] = 1;
    }
}

void sim_set_running(int on) {
    sim_running = on ? 1 : 0;
    dragging_id = -1;
    wiring_active = 0;
    sim_recompute();
    if (sim_running) {
        if (!sim_has_battery && !sim_has_ac)
            set_status(L"RUN: pasang Baterai atau Sumber AC dulu, lalu sambungkan kabel.");
        else set_status(L"RUN: %ls", sim_message());
    } else {
        set_status(L"Mode Edit. Klik ikon di toolbar, lalu klik canvas.");
    }
}

int sim_wire_hot(int idx) {
    if (idx < 0 || idx >= wire_count) return 0;
    return hot_[idx];
}

int sim_comp_lit(const Component *c) {
    if (!c) return 0;
    int i = (int)(c - comps);
    if (i < 0 || i >= comp_count) return 0;
    /* LED yang meledak tetap "menyala" (cahaya terang) sebagai indikasi visual */
    return lit_[i];
}

int sim_diode_fwd(const Component *c) {
    if (!c || c->type != COMP_DIODE) return 0;
    int i = (int)(c - comps);
    if (i < 0 || i >= comp_count) return 0;
    return dfwd_[i];
}

int sim_comp_conducting(const Component *c) {
    if (!c || c->type != COMP_TRANSISTOR) return 0;
    int i = (int)(c - comps);
    if (i < 0 || i >= comp_count) return 0;
    return cond_[i];
}

int sim_buzzer_on(const Component *c) {
    if (!c || c->type != COMP_BUZZER) return 0;
    int i = (int)(c - comps);
    if (i < 0 || i >= comp_count) return 0;
    if (!sim_running) return 0;
    return buzz_[i];
}

double sim_ldr_effective_r(const Component *c) {
    if (!c || c->type != COMP_LDR) return -1.0;
    double rdark = (c->v > 0.0) ? c->v : LDR_DARK_DEFAULT;
    /* on = 1 => TERANG: R ~ 1% nilai gelap; on = 0 => GELAP: R penuh */
    return c->on ? rdark * LDR_LIGHT_RATIO : rdark;
}

/* Bacaan voltmeter (volt). Kontrak (lihat schematic.h):
   -1 = bukan voltmeter, mode EDIT, atau salah satu pin mengambang;
   kedua pin pada net yang sama -> 0 V; kutub terbalik -> negatif. */
double sim_voltmeter_reading(const Component *c) {
    if (!c || c->type != COMP_VOLTMETER) return -1.0;
    int i = (int)(c - comps);
    if (i < 0 || i >= comp_count) return -1.0;
    if (!vmconn_[i]) return -1.0;            /* tak terhubung penuh */
    return vmread_[i];
}

int sim_voltmeter_connected(const Component *c) {
    if (!c || c->type != COMP_VOLTMETER) return 0;
    int i = (int)(c - comps);
    if (i < 0 || i >= comp_count) return 0;
    return vmconn_[i];
}

/* Format angka SI untuk pesan: "31.8 mA", "220 Ohm", "9 V" */
void sim_format_si(double v, const wchar_t *unit, wchar_t *out, int cap) {
    const wchar_t *pre = L"";
    double a = fabs(v);
    if (a >= 1e3)      { v /= 1e3; pre = L"k"; }
    else if (a >= 1.0) { /* tetap */ }
    else if (a >= 1e-3){ v /= 1e-3; pre = L"m"; }
    else if (a >= 1e-6){ v /= 1e-6; pre = L"u"; }
    else if (a >= 1e-9){ v /= 1e-9; pre = L"n"; }
    else               { v /= 1e-12; pre = L"p"; }
    swprintf(out, cap, L"%.3g %ls%s", v, pre, unit ? unit : L"");
}

int sim_comp_blown(const Component *c) {
    if (!c) return 0;
    int i = (int)(c - comps);
    if (i < 0 || i >= comp_count) return 0;
    return blown_[i];
}

double sim_comp_current(const Component *c) {
    if (!c) return -1.0;
    int i = (int)(c - comps);
    if (i < 0 || i >= comp_count) return -1.0;
    return ilead_[i];
}

static wchar_t g_msg[256];
const wchar_t *sim_message(void) {
    wchar_t vi[32], vr[32];
    if (!sim_has_battery && !sim_has_ac)
        return L"Belum ada sumber (baterai/AC) — LED tidak bisa menyala.";
    if (sim_short) return L"KORSLETING! (+) tersambung langsung ke (-).";
    if (sim_exploded) {
        sim_format_si(sim_iled_max, L"A", vi, 32);
        swprintf(g_msg, 256,
            L"BAHAYA! LED MELEDAK — arus loop %ls jauh melebihi batas "
            L"20 mA. Tambah/perbesar resistor seri!", vi);
        return g_msg;
    }
    if (sim_overcur) {
        sim_format_si(sim_iled_max, L"A", vi, 32);
        swprintf(g_msg, 256,
            L"PERINGATAN! LED rusak pelan — arus %ls > 20 mA (max). "
            L"Perbesar resistor seri.", vi);
        return g_msg;
    }
    if (sim_led_count > 0) {
        /* Arus pada loop seri: I = (V_sumber - total Vf) / R_loop */
        double dv = sim_vsrc - sim_vdrop_max;
        double ileff = (dv < 0.0) ? 0.0 : dv;
        wchar_t vd[32];
        sim_format_si(ileff, L"A", vi, 32);
        sim_format_si(sim_rloop, L"\x03A9", vr, 32);
        sim_format_si(sim_vdrop_max, L"V", vd, 32);
        if (sim_ac_mode) {
            swprintf(g_msg, 256,
                L"Rangkaian AC BENAR — LED MENYALA (pendekatan RMS). "
                L"I = (VAC - Vf_total)/R = (%.4g - %ls)/%ls = %ls. AMAN.",
                sim_vsrc, vd, vr, vi);
        } else {
            swprintf(g_msg, 256,
                L"Rangkaian BENAR — LED MENYALA. "
                L"I = (Vbat - Vf_total)/R = (%.4g - %ls)/%ls = %ls. AMAN.",
                sim_vsrc, vd, vr, vi);
        }
        return g_msg;
    }
    return L"Rangkaian belum lengkap — LED mati. Tutup saklar & sambungkan kabel.";
}
