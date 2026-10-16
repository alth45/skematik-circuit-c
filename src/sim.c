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

/* BFS dari node `start` mengikuti kabel saja (union tanpa komponen).
   Mengisi nrk_/qn_ (daftar net satu untaian, max *nn_out), dan head[] =
   index komponen dua-pin yang menyentuh tiap net (-1 = buntu). */
static void net_heads_from(int start, int *head, int nhcap, int *nn_out) {
    for (int i = 0; i < nhcap; i++) head[i] = -1;
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
            int na = node_of(ia, wires[w].a.pin_idx);
            int nb = node_of(ib, wires[w].b.pin_idx);
            int ra = find_root(na), rb = find_root(nb);
            if (ra == r && rb != r) net_find_or_add(rb, qn_, &nn, NETQ_MAX);
            if (rb == r && ra != r) net_find_or_add(ra, qn_, &nn, NETQ_MAX);
        }
    }
    for (int i = 0; i < comp_count; i++) {
        int n = comp_pin_count(&comps[i]);
        if (n < 2) continue;
        int h0 = -1, h1 = -1;
        for (int k = 0; k < nn; k++) {
            if (find_root(node_of(i, 0)) == nrk_[k]) h0 = k;
            if (find_root(node_of(i, 1)) == nrk_[k]) h1 = k;
        }
        if (h0 >= 0 && h1 >= 0 && h0 != h1) { head[h0] = i; head[h1] = i; }
    }
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


    /* ---- Pass 3: PENYALAAN LED + PERHITUNGAN NILAI (Hukum Ohm & Kirchhoff).
       Untuk tiap LED yang terhubung benar (anoda ke net (+), katoda ke
       net (-)), telusuri jalur dari anoda kembali ke kutub (+):
         R_total = jumlah resistor sepanjang jalur
         V_drops = Vf LED + Vf dioda forward + 0.2 V transistor saturasi
         I = (V_sumber - V_drops) / R_total
       V_sumber = tegangan sumber pada LOOP LED itu sendiri (KVL): sumber
       yang kutub (+)-nya berada pada untaian net yang sama dengan anoda
       LED. Dulu dipakai maksimum GLOBAL seluruh rangkaian — itu salah:
       misal baterai 9 V + LED+R (aman), lalu ditambahkan sumber AC 220 V
       terpisah -> loop LED yang sudah benar tetap dituduh "MELEDAK".
       Bila tidak ada sumber pada untaian LED, fallback: baterai terbesar,
       lalu AC terbesar (RMS). Kapasitor dianggap tembus hanya pada mode AC.
       Tanpa resistor pengaman -> arus praktis tak hingga -> LED MELEDAK.
       Percabangan paralel -> arus TERBESAR dipakai (konservatif). ---- */
    {
        static int head_[NETQ_MAX];
        static int nn_led[MAX_COMP];      /* jumlah net untaian per LED */
        static int start_net_[MAX_COMP];  /* indeks net awal per LED */
        double vsrc_max = 0.0;
        for (int i = 0; i < comp_count; i++) {
            if (comps[i].type == COMP_BATTERY && comps[i].v > vsrc_max) {
                vsrc_max = comps[i].v;
            }
        }
        if (vsrc_max <= 0.0) {
            /* tak ada DC: pakai sumber AC terbesar (RMS) */
            for (int i = 0; i < comp_count; i++) {
                if (comps[i].type == COMP_ACSOURCE && comps[i].v > vsrc_max) {
                    vsrc_max = comps[i].v;
                }
            }
        }
        sim_vsrc = vsrc_max;

        for (int i = 0; i < comp_count; i++) {
            if (comps[i].type != COMP_LED) continue;
            int r0 = find_root(node_of(i, 0));
            int r1 = find_root(node_of(i, 1));
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

            /* bangun untaian net dari anoda mengikuti kabel saja */
            int nn = 0;
            net_heads_from(node_of(i, 0), head_, NETQ_MAX, &nn);
            int sn = net_index_of(r0, nn);
            if (sn < 0) { nn_led[i] = 0; start_net_[i] = -1; continue; }
            nn_led[i] = nn;
            start_net_[i] = sn;
            (void)nn_led[i]; (void)start_net_[i]; /* info debug per-LED, non-fatal */

            double rsum = 0.0;
            double vdrop = (comps[i].v > 0.0 ? comps[i].v : VF_LED_DEFAULT);
            int cur_net = sn, guard = 0;
            int found_path = 0;
            for (int k = 0; k < n_plus; k++)
                if (plus_root[k] == nrk_[cur_net]) { found_path = 1; break; }
            while (!found_path && guard++ < nn) {
                int h = head_[cur_net];
                if (h < 0) break;                    /* buntu */
                Component *hc = &comps[h];
                int other_pin;
                if (hc->type == COMP_RESISTOR) {
                    rsum += hc->v;
                } else if (hc->type == COMP_LDR) {
                    /* R efektif ikut cahaya: TERANG ~1% R gelap */
                    double reff = sim_ldr_effective_r(hc);
                    rsum += (reff >= 0.0) ? reff : 0.0;
                } else if (hc->type == COMP_BUZZER) {
                    /* impedansi buzzer diabaikan (ideal) */
                } else if (hc->type == COMP_DIODE && dfwd_[h]) {
                    vdrop += (hc->v > 0.0 ? hc->v : VF_DIODE);
                } else if (hc->type == COMP_SWITCH && hc->on) {
                    /* ideal: tanpa drop */
                } else if (hc->type == COMP_TRANSISTOR && cond_[h]) {
                    vdrop += 0.2;                    /* V_CE saturasi */
                } else if (sim_ac_mode &&
                           (hc->type == COMP_CAPACITOR || hc->type == COMP_CAPOLAR)) {
                    /* ideal AC: kapasitor tembus tanpa drop */
                } else {
                    break;   /* baterai/ground/kapasitor/dioda reverse -> stop */
                }
                other_pin = (find_root(node_of(h, 0)) == nrk_[cur_net]) ? 1 : 0;
                int oroot = find_root(node_of(h, other_pin));
                for (int k = 0; k < n_plus; k++)
                    if (plus_root[k] == oroot) { found_path = 1; break; }
                if (found_path) break;
                int idx2 = net_index_of(oroot, nn);
                if (idx2 < 0) break;                 /* keluar untaian -> buntu */
                cur_net = idx2;
            }

            double iled;
            /* Tegangan sumber utk LED ini: sumber yang kutub (+)-nya duduk
               pada untaian net anoda LED (loop KVL yang sama). Kalau tidak
               ketemu (mis. (+) baterai baru tersambung ke net anoda lewat
               dioda/transistor), pakai baterai terbesar, lalu AC terbesar. */
            double vled = 0.0;
            for (int s = 0; s < comp_count; s++) {
                if (comps[s].type != COMP_BATTERY && comps[s].type != COMP_ACSOURCE)
                    continue;
                int rp = find_root(node_of(s, 0));
                int inChain = 0;
                for (int k = 0; k < nn; k++) if (nrk_[k] == rp) { inChain = 1; break; }
                if (!inChain) continue;
                double vv = comps[s].v;
                if (vv <= 0.0) vv = (comps[s].type == COMP_ACSOURCE) ? 220.0 : 9.0;
                if (vv > vled) vled = vv;
            }
            if (vled <= 0.0) vled = vsrc_max;   /* fallback global */
            if (!found_path)      iled = 1e9;        /* jalur tak lengkap -> korslet */
            else if (rsum <= 0.0) iled = 1e9;        /* tanpa R = arus tak hingga */
            else {
                double dv = vled - vdrop;
                iled = (dv > 0.0) ? dv / rsum : 0.0; /* Vf >= Vbat -> tak menyala */
            }
            ilead_[i] = iled;
            vdrop_[i] = vdrop;
            if (iled > sim_iled_max) {
                sim_iled_max = iled; sim_rloop = rsum;
                sim_vsrc = vled; sim_vdrop_max = vdrop;
            } else if (sim_iled_max >= 1e8 && iled < 1e8) {
                /* ada LED korslet (arus "tak hingga") — tetap tampilkan
                   angka loop LED yang benar-benar tersambung */
                sim_iled_max = iled; sim_rloop = rsum;
                sim_vsrc = vled; sim_vdrop_max = vdrop;
            }

            /* Nasib LED (hukum Ohm, dengan toleransi pembulatan):
               - I praktis tak hingga (tanpa R / jalur cacat) -> MELEDAK
               - I > 5x If_max (100 mA) atau V_sumber loop > 50 V -> MELEDAK
               - If_max < I <= 5x If_max -> rusak pelan (overcurrent)
               Perbandingan memakai epsilon relatif: nilai IDEAL I = 20 mA
               (mis. (9-2)V / 350Ω) harus selalu dinyatakan AMAN, bukan
               terkena galat pembulatan floating-point. */
            const double tol = 1.0 + 1e-6;
            if (iled >= 1e8 || iled > 5.0 * VF_LED_MAX * tol ||
                (iled > VF_LED_MAX * tol && vled > V_EXPLODE)) {
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
