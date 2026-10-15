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
int    sim_exploded = 0;
int    sim_overcur = 0;

/* Net listrik: tiap pin komponen = 1 node.
   node = comp_index * 3 + pin (ground pin1/2, basis = pin2 tak dipakai
   komponen 1-2 pin; transistor pakai pin0=C, pin1=E, pin2=B). */
static int parent_[MAX_COMP * 3];
static int lit_[MAX_COMP];
static int cond_[MAX_COMP];
static int dfwd_[MAX_COMP];   /* 1 = dioda menghantar forward (A->K) */
static int hot_[MAX_WIRES];
static int blown_[MAX_COMP];  /* 1 = LED meledak (kelebihan arus/tegangan) */
static double ilead_[MAX_COMP]; /* arus hasil hitung per komponen (A); -1 = tak terdefinisi */

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
        lit_[i] = 0; cond_[i] = 0; dfwd_[i] = 0; blown_[i] = 0; ilead_[i] = -1.0;
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
       V_sumber = baterai DC bila ada; bila hanya sumber AC, dipakai nilai
       RMS AC (mode AC). Kapasitor dianggap tembus hanya pada mode AC.
       Tanpa resistor pengaman -> arus praktis tak hingga -> LED MELEDAK.
       Percabangan paralel -> arus TERBESAR dipakai (konservatif). ---- */
    {
        static int head_[NETQ_MAX];
        static int nn_led[MAX_COMP];      /* jumlah net untaian per LED */
        static int start_net_[MAX_COMP];  /* indeks net awal per LED */
        double vsrc_max = 0.0;
        int vsrc_is_ac = 0;
        for (int i = 0; i < comp_count; i++) {
            if (comps[i].type == COMP_BATTERY && comps[i].v > vsrc_max) {
                vsrc_max = comps[i].v;
                vsrc_is_ac = 0;
            }
        }
        if (vsrc_max <= 0.0) {
            /* tak ada DC: pakai sumber AC terbesar (RMS) */
            for (int i = 0; i < comp_count; i++) {
                if (comps[i].type == COMP_ACSOURCE && comps[i].v > vsrc_max) {
                    vsrc_max = comps[i].v;
                    vsrc_is_ac = 1;
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

            /* bangun untaian net dari anoda mengikuti kabel saja */
            int nn = 0;
            net_heads_from(node_of(i, 0), head_, NETQ_MAX, &nn);
            int sn = net_index_of(r0, nn);
            if (sn < 0) { nn_led[i] = 0; start_net_[i] = -1; continue; }
            nn_led[i] = nn;
            start_net_[i] = sn;

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
            if (!found_path)      iled = 1e9;        /* jalur tak lengkap -> korslet */
            else if (rsum <= 0.0) iled = 1e9;        /* tanpa R = arus tak hingga */
            else {
                double dv = vsrc_max - vdrop;
                iled = (dv > 0.0) ? dv / rsum : 0.0; /* Vf >= Vbat -> tak menyala */
            }
            ilead_[i] = iled;
            if (iled > sim_iled_max) { sim_iled_max = iled; sim_rloop = rsum; }

            /* Nasib LED:
               - I tak hingga / > 5x If_max (100 mA) / Vbat > 50 V -> MELEDAK
               - If_max < I <= 5x If_max -> rusak pelan (overcurrent) */
            if (iled >= 1e8 || iled > 5.0 * VF_LED_MAX ||
                (iled > VF_LED_MAX && vsrc_max > V_EXPLODE)) {
                blown_[i] = 1;
                sim_exploded = 1;
            } else if (iled > VF_LED_MAX) {
                blown_[i] = 1;
                sim_overcur = 1;
            }
        }
    }

    for (int i = 0; i < wire_count; i++) {
        Component *ca = find_comp_by_id(wires[i].a.comp_id);
        if (!ca) continue;
        int ia = (int)(ca - comps);
        if (ia < 0 || ia >= comp_count) continue;
        int r = find_root(node_of(ia, wires[i].a.pin_idx));
        for (int k = 0; k < n_plus; k++)
            if (plus_root[k] == r) { hot_[i] = 1; break; }
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
            L"BAHAYA! LED MELEDAK — arus %ls melebihi batas 20 mA. "
            L"Tambah/perbesar resistor seri!", vi);
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
        sim_format_si(sim_iled_max, L"A", vi, 32);
        sim_format_si(sim_rloop, L"\x03A9", vr, 32);
        if (sim_ac_mode) {
            swprintf(g_msg, 256,
                L"Rangkaian AC BENAR — LED MENYALA (pendekatan RMS). I = %ls, R_loop = %ls, VAC = %.4g V.",
                vi, vr, sim_vsrc);
        } else {
            swprintf(g_msg, 256,
                L"Rangkaian BENAR — LED MENYALA. I = %ls, R_loop = %ls, Vbat = %.4g V.",
                vi, vr, sim_vsrc);
        }
        return g_msg;
    }
    return L"Rangkaian belum lengkap — LED mati. Tutup saklar & sambungkan kabel.";
}
