/* ============================================================================
   sim.c — Simulasi rangkaian: analisis nodal (MNA-lite) + hukum Ohm/KVL.

   Arsitektur:
     1. Union-Find untuk kabel + penghantar IDEAL 0 Ω (saklar-ON, dioda-fwd,
        transistor-ON, kapasitor mode-AC-murni).
     2. Resistor / LDR / buzzer / kapasitor-AC  → EDGE berbobot R.
     3. Sumber tegangan → node dengan V TETAP (fixed), bukan edge.
     4. Untuk tiap LED:
          - Selesaikan KCL global → V_thevenin (tegangan open di anoda).
          - Selesaikan lagi (semua sumber di-short, injeksi +1 A di anoda,
            −1 A di katoda) → R_thevenin.
          - I_LED = (Vth - Vf) / Rth.
   Presisi: 1% untuk topologi seri / paralel / pembagi tegangan tipikal.
============================================================================ */
#include "schematic.h"

/* ---- State global (kontrak schematic.h) ---- */
int sim_running = 0;
int sim_led_count = 0;
int sim_short = 0;
int sim_has_battery = 0;
int sim_has_ac = 0;
int sim_ac_mode = 0;

double sim_vsrc = 0.0;
double sim_rloop = 0.0;
double sim_iled_max = 0.0;
double sim_vdrop_max = 0.0;
int sim_exploded = 0;
int sim_overcur = 0;

/* ---- Konstanta model listrik ---- */
#define BUZZER_R_DEFAULT 100.0 /* impedansi buzzer tipikal (Ohm) */
#define R_AMMETER_OHM 0.1
#define IND_DEFAULT_H_AC 100e-6  /* fallback L bila c->v <= 0 (AC edge) */
// #define IND_DEFAULT 100e-6     /* fallback L bila c->v <= 0 (DC edge) */
#define AC_FREQ_HZ 50.0        /* frekuensi sumber AC (Hz) */
#define CAP_DEFAULT_F 100e-9   /* kapasitansi default bila v<=0 */
#define R_SHORT_THRESH 0.01    /* < ini dianggap short (Ohm) */
#define R_OPEN_THRESH 1e9      /* > ini dianggap loop terbuka (Ohm) */
#define G_REG 1e-12            /* regularisasi konduktansi ke ground (S) */
#ifdef V_UNKNOWN
#undef V_UNKNOWN
#endif
#define V_UNKNOWN 1e30         /* penanda node "belum ter-fix" (internal sim.c) */
#define VM_DISCONNECTED (-1e30)

/* ---- State per-komponen ---- */
static int parent_[MAX_COMP * 3];
static int lit_[MAX_COMP];
static int cond_[MAX_COMP];
static int dfwd_[MAX_COMP];
static int buzz_[MAX_COMP];
static int blown_[MAX_COMP];
static double ilead_[MAX_COMP];
static double vdrop_[MAX_COMP];
static double vmread_[MAX_COMP];
static int vmconn_[MAX_COMP];
static int hot_[MAX_WIRES];
static double amread_[MAX_COMP];
static int zener_bd_[MAX_COMP];   /* 1 = Zener sedang reverse breakdown */
/* ============================================================================
   Bagian 1: Union-Find & helper node
============================================================================ */
static int find_root(int a) {
  while (parent_[a] != a) {
    parent_[a] = parent_[parent_[a]];
    a = parent_[a];
  }
  return a;
}
static void unite(int a, int b) {
  int ra = find_root(a), rb = find_root(b);
  if (ra != rb)
    parent_[ra] = rb;
}
static int node_of(int ci, int pin) { return ci * 3 + pin; }

/* ============================================================================
   Bagian 2: Matriks nodal — Gauss solver
============================================================================ */
#define NA_MAX (MAX_COMP * 3) /* 768 */

static double na_A[NA_MAX][NA_MAX];
static double na_b[NA_MAX];
static double na_x[NA_MAX];
static int na_n = 0;
static int root_to_idx_[MAX_COMP * 3];
static int idx_to_root_[NA_MAX];

/* Bangun daftar node (= root unik yang dipakai pin komponen). */
static void na_build_nodes(void) {
  for (int i = 0; i < MAX_COMP * 3; i++)
    root_to_idx_[i] = -1;
  na_n = 0;
  for (int i = 0; i < comp_count; i++) {
    int np = comp_pin_count(&comps[i]);
    for (int p = 0; p < np; p++) {
      int r = find_root(node_of(i, p));
      if (r < 0 || r >= MAX_COMP * 3)
        continue;
      if (root_to_idx_[r] < 0 && na_n < NA_MAX) {
        root_to_idx_[r] = na_n;
        idx_to_root_[na_n] = r;
        na_n++;
      }
    }
  }
}

/* ============================================================
   Sumber nilai untuk na_assign_fixed_mode().
   - SRC_MODE_NORMAL    : pakai comps[i].v (perilaku lama).
   - SRC_MODE_DC_ONLY   : sumber AC = 0, sumber DC = c->v.
   - SRC_MODE_ZERO_ALL  : semua sumber = 0.
   - SRC_MODE_UNIT_ONE  : hanya sumber index `target` = 1 V, sisanya = 0.
     (dipakai untuk menghitung gain transfer probe → AC source)
============================================================ */
typedef enum {
    SRC_MODE_NORMAL = 0,
    SRC_MODE_DC_ONLY,
    SRC_MODE_ZERO_ALL,
    SRC_MODE_UNIT_ONE
} SrcMode;

static double src_val(int idx, SrcMode mode, int target) {
    int is_ac = (comps[idx].type == COMP_ACSOURCE);
    int is_dc = (comps[idx].type == COMP_BATTERY);
    if (!is_ac && !is_dc) return 0.0;

    switch (mode) {
        case SRC_MODE_NORMAL: {
            double v = comps[idx].v;
            if (v <= 0.0) v = is_ac ? 220.0 : 9.0;
            return v;
        }
        case SRC_MODE_DC_ONLY: {
            if (is_ac) return 0.0;
            double v = comps[idx].v;
            if (v <= 0.0) v = 9.0;
            return v;
        }
        case SRC_MODE_ZERO_ALL:
            return 0.0;
        case SRC_MODE_UNIT_ONE:
            return (idx == target) ? 1.0 : 0.0;
    }
    return 0.0;
}

static int is_dc_source(int idx) {
    return comps[idx].type == COMP_BATTERY;
}
static int is_ac_source(int idx) {
    return comps[idx].type == COMP_ACSOURCE;
}
static int is_zener_bd(int idx) {
    return comps[idx].type == COMP_ZENER && zener_bd_[idx];
}

static double zener_val(int idx, SrcMode mode) {
    if (mode == SRC_MODE_ZERO_ALL || mode == SRC_MODE_UNIT_ONE) return 0.0;
    return (comps[idx].v > 0.0) ? comps[idx].v : VZENER_DEFAULT;
}

/* Versi baru: parameter lebih rinci untuk tiap jenis sumber. */
static void na_assign_fixed_mode(double *vfix, SrcMode mode, int target) {
  for (int i = 0; i < na_n; i++) vfix[i] = V_UNKNOWN;

  /* Pilih node ground: sumber (termasuk Zener breakdown) dengan |V| terbesar.
     Sisi "−" dari sumber tsb = ground. */
  int ground_idx = -1;
  double best_v = -1.0;
  for (int i = 0; i < comp_count; i++) {
    double vv;
    int neg_pin;
    if (is_zener_bd(i)) {
      vv = fabs(zener_val(i, mode));
      neg_pin = 0;                                 /* A = kutub "−" Zener */
    } else if (is_ac_source(i) || is_dc_source(i)) {
      vv = fabs(src_val(i, mode, target));
      neg_pin = 1;                                 /* pin1 = "−" sumber    */
    } else {
      continue;
    }
    if (vv > best_v) {
      best_v = vv;
      int rm = find_root(node_of(i, neg_pin));
      if (rm >= 0 && rm < MAX_COMP * 3) ground_idx = root_to_idx_[rm];
    }
  }
  if (ground_idx < 0) {
    for (int i = 0; i < comp_count; i++) {
      if (comps[i].type != COMP_GROUND) continue;
      int rg = root_to_idx_[find_root(node_of(i, 0))];
      if (rg >= 0) { ground_idx = rg; break; }
    }
  }
  if (ground_idx >= 0) vfix[ground_idx] = 0.0;

  /* Propagasi. */
  int changed = 1, iter = 0;
  while (changed && iter <= na_n + 2) {
    changed = 0;
    iter++;
    for (int i = 0; i < comp_count; i++) {
      double vv;
      int pos_pin, neg_pin;
      if (is_zener_bd(i)) {
        vv = zener_val(i, mode);
        pos_pin = 1;                                /* K = "+" untuk Zener */
        neg_pin = 0;
      } else if (is_ac_source(i) || is_dc_source(i)) {
        vv = src_val(i, mode, target);
        pos_pin = 0;
        neg_pin = 1;
      } else {
        continue;
      }
      int ip = root_to_idx_[find_root(node_of(i, pos_pin))];
      int im = root_to_idx_[find_root(node_of(i, neg_pin))];
      if (ip < 0 || im < 0 || ip == im) continue;
      if (vfix[im] != V_UNKNOWN && vfix[ip] == V_UNKNOWN) {
        vfix[ip] = vfix[im] + vv;
        changed = 1;
      } else if (vfix[ip] != V_UNKNOWN && vfix[im] == V_UNKNOWN) {
        vfix[im] = vfix[ip] - vv;
        changed = 1;
      }
    }
  }

  /* Sumber yatim (kedua sisi belum ter-fix). */
  for (int i = 0; i < comp_count; i++) {
    double vv;
    int pos_pin, neg_pin;
    if (is_zener_bd(i)) {
      vv = zener_val(i, mode);
      pos_pin = 1; neg_pin = 0;
    } else if (is_ac_source(i) || is_dc_source(i)) {
      vv = src_val(i, mode, target);
      pos_pin = 0; neg_pin = 1;
    } else {
      continue;
    }
    int ip = root_to_idx_[find_root(node_of(i, pos_pin))];
    int im = root_to_idx_[find_root(node_of(i, neg_pin))];
    if (ip < 0 || im < 0 || ip == im) continue;
    if (vfix[ip] == V_UNKNOWN && vfix[im] == V_UNKNOWN) {
      vfix[im] = 0.0;
      vfix[ip] = vv;
    }
  }

  /* Ground komponen. */
  for (int i = 0; i < comp_count; i++) {
    if (comps[i].type != COMP_GROUND) continue;
    int rg = root_to_idx_[find_root(node_of(i, 0))];
    if (rg >= 0 && vfix[rg] == V_UNKNOWN) vfix[rg] = 0.0;
  }
}
/* Wrapper kompat: perilaku lama. */
static void na_assign_fixed(double *vfix) {
  na_assign_fixed_mode(vfix, SRC_MODE_NORMAL, -1);
}



/* Resistansi efektif komponen sebagai EDGE. Return -1 = bukan edge resistif. */
static double comp_edge_r(int i) {
  const Component *c = &comps[i];
  switch (c->type) {
  case COMP_RESISTOR:
    return (c->v > 0.0) ? c->v : 0.001;
  case COMP_LDR: {
    double r = sim_ldr_effective_r(c);
    return (r > 0.0) ? r : 0.001;
  }
  case COMP_BUZZER:
    return BUZZER_R_DEFAULT;
  case COMP_CAPACITOR:
  case COMP_CAPOLAR:
    if (sim_ac_mode) {
      double cf = (c->v > 0.0) ? c->v : CAP_DEFAULT_F;
      double xc = 1.0 / (2.0 * M_PI * AC_FREQ_HZ * cf);
      return (xc > 0.0) ? xc : 0.001;
    }
    return -1.0; /* blokir DC */
  case COMP_AMMETER:
    return 0.001; /* resistansi sangat kecil */
  case COMP_INDUCTOR:
    if (sim_ac_mode) {
      double lh = (c->v > 0.0) ? c->v : IND_DEFAULT_H_AC;
      double xl = 2.0 * M_PI * AC_FREQ_HZ * lh;
      return (xl > 0.0) ? xl : 0.001;
    }
    return -1.0; /* DC: ditangani di Pass 1 sebagai short */
  default:
    return -1.0;
  }
}

/* Bangun matriks G & vektor b untuk solve KCL.
   vfix[]    : tegangan tetap tiap node (V_UNKNOWN = solve).
   inject    : index node sumber arus +1 A (atau -1 untuk ekstraksi); -1 = tidak
   ada. extract   : index node sumber arus -1 A; -1 = tidak ada. */
static void na_build(const double *vfix, int inject, int extract) {
  for (int i = 0; i < na_n; i++) {
    for (int j = 0; j < na_n; j++)
      na_A[i][j] = 0.0;
    na_b[i] = 0.0;
  }
  char is_fixed[NA_MAX];
  for (int i = 0; i < na_n; i++)
    is_fixed[i] = (vfix[i] != V_UNKNOWN);

  /* Edge resistif (R / LDR / buzzer / cap-AC). */
  for (int c = 0; c < comp_count; c++) {
    double R = comp_edge_r(c);
    if (R < 0.0)
      continue;
    int u = root_to_idx_[find_root(node_of(c, 0))];
    int v = root_to_idx_[find_root(node_of(c, 1))];
    if (u < 0 || v < 0 || u == v)
      continue;
    double g = 1.0 / R;
    if (!is_fixed[u] && !is_fixed[v]) {
      na_A[u][u] += g;
      na_A[v][v] += g;
      na_A[u][v] -= g;
      na_A[v][u] -= g;
    } else if (!is_fixed[u]) {
      na_A[u][u] += g;
      na_b[u] += g * vfix[v];
    } else if (!is_fixed[v]) {
      na_A[v][v] += g;
      na_b[v] += g * vfix[u];
    }
  }

  /* Regularisasi: konduktansi kecil ke ground untuk node mengambang. */
  for (int i = 0; i < na_n; i++) {
    if (!is_fixed[i])
      na_A[i][i] += G_REG;
  }

  /* Injeksi arus (untuk hitung R_thevenin). */
  if (inject >= 0 && inject < na_n && !is_fixed[inject])
    na_b[inject] += 1.0;
  if (extract >= 0 && extract < na_n && !is_fixed[extract])
    na_b[extract] -= 1.0;

  /* Baris constraint untuk node V-fixed. */
  for (int i = 0; i < na_n; i++) {
    if (is_fixed[i]) {
      for (int j = 0; j < na_n; j++)
        na_A[i][j] = 0.0;
      na_A[i][i] = 1.0;
      na_b[i] = vfix[i];
    }
  }
}

/* Gauss elimination dengan partial pivoting. Return 1 sukses. */
static int na_solve(void) {
  int n = na_n;
  for (int k = 0; k < n; k++) {
    int piv = k;
    for (int i = k + 1; i < n; i++)
      if (fabs(na_A[i][k]) > fabs(na_A[piv][k]))
        piv = i;
    if (fabs(na_A[piv][k]) < 1e-18)
      return 0; /* singular */
    if (piv != k) {
      for (int j = k; j < n; j++) {
        double t = na_A[k][j];
        na_A[k][j] = na_A[piv][j];
        na_A[piv][j] = t;
      }
      double t = na_b[k];
      na_b[k] = na_b[piv];
      na_b[piv] = t;
    }
    for (int i = k + 1; i < n; i++) {
      double f = na_A[i][k] / na_A[k][k];
      if (fabs(f) < 1e-30)
        continue;
      for (int j = k; j < n; j++)
        na_A[i][j] -= f * na_A[k][j];
      na_b[i] -= f * na_b[k];
    }
  }
  for (int i = n - 1; i >= 0; i--) {
    double s = na_b[i];
    for (int j = i + 1; j < n; j++)
      s -= na_A[i][j] * na_x[j];
    na_x[i] = s / na_A[i][i];
  }
  return 1;
}

/* ============================================================================
   Bagian 3: sim_recompute — orkestrator utama
============================================================================ */
void sim_recompute(void) {
  /* ---- Reset state ---- */
  {extern int _sim_probe_resp_ready; _sim_probe_resp_ready = 0;}

  for (int i = 0; i < MAX_COMP * 3; i++)
    parent_[i] = i;
  for (int i = 0; i < MAX_COMP; i++) {
    lit_[i] = 0;
    cond_[i] = 0;
    dfwd_[i] = 0;
    buzz_[i] = 0;
    blown_[i] = 0;
    ilead_[i] = -1.0;
    vdrop_[i] = 0.0;
    vmread_[i] = VM_DISCONNECTED;
    vmconn_[i] = 0;
    amread_[i] = -1.0;
    zener_bd_[i] = 0;
  }
  for (int i = 0; i < MAX_WIRES; i++)
    hot_[i] = 0;
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

  /* ---- Union kabel ---- */
  for (int i = 0; i < wire_count; i++) {
    Component *ca = find_comp_by_id(wires[i].a.comp_id);
    Component *cb = find_comp_by_id(wires[i].b.comp_id);
    if (!ca || !cb)
      continue;
    int ia = (int)(ca - comps), ib = (int)(cb - comps);
    if (ia < 0 || ia >= comp_count || ib < 0 || ib >= comp_count)
      continue;
    unite(node_of(ia, wires[i].a.pin_idx), node_of(ib, wires[i].b.pin_idx));
  }

  /* ---- Deteksi sumber ---- */
  int has_ac = 0, has_dc = 0;
  for (int i = 0; i < comp_count; i++) {
    if (comps[i].type == COMP_ACSOURCE)
      has_ac = 1;
    if (comps[i].type == COMP_BATTERY)
      has_dc = 1;
  }
  sim_has_ac = has_ac;
  sim_has_battery = has_dc;
  sim_ac_mode = (has_ac && !has_dc);

  /* ---- Pass 1: union penghantar IDEAL 0 Ω saja (BUKAN resistor). ---- */
  for (int i = 0; i < comp_count; i++) {
    if (comps[i].type == COMP_SWITCH && comps[i].on) {
      unite(node_of(i, 0), node_of(i, 1));
    }
    if (comps[i].type == COMP_INDUCTOR && !sim_ac_mode) {
      unite(node_of(i, 0), node_of(i, 1));
    }
    /* Kapasitor di mode murni-AC: dianggap short (Xc besar dimodelkan
       sebagai edge, BUKAN short) — jadi di sini TIDAK di-union. */
  }

  /* Short deteksi (+)-(−). */
  {
    int plus_root[MAX_COMP * 3], minus_root[MAX_COMP * 3];
    int n_plus = 0, n_minus = 0;
    for (int i = 0; i < comp_count; i++) {
      if (comps[i].type == COMP_BATTERY || comps[i].type == COMP_ACSOURCE) {
        plus_root[n_plus++] = find_root(node_of(i, 0));
        minus_root[n_minus++] = find_root(node_of(i, 1));
      } else if (comps[i].type == COMP_GROUND) {
        minus_root[n_minus++] = find_root(node_of(i, 0));
      }
    }
    for (int a = 0; a < n_plus; a++)
      for (int b = 0; b < n_minus; b++)
        if (plus_root[a] == minus_root[b])
          sim_short = 1;
  }

  /* ---- Pass 1b: dioda fwd (union A-K) ---- */
  {
    int di_on[MAX_COMP] = {0};
    int plus_root[MAX_COMP * 3], n_plus = 0;
    for (int i = 0; i < comp_count; i++) {
      if (comps[i].type == COMP_BATTERY || comps[i].type == COMP_ACSOURCE)
        plus_root[n_plus++] = find_root(node_of(i, 0));
    }
    for (int i = 0; i < comp_count; i++) {
      if (comps[i].type != COMP_DIODE && comps[i].type != COMP_ZENER)
        continue;
      int ra = find_root(node_of(i, 0));
      for (int k = 0; k < n_plus; k++)
        if (plus_root[k] == ra) {
          di_on[i] = 1;
          break;
        }
    }
    for (int i = 0; i < comp_count; i++)
      if (di_on[i]) {
        dfwd_[i] = 1;
        unite(node_of(i, 0), node_of(i, 1));
      }
  }

  /* ---- Pass 2: transistor ON (union C-E) ---- */
  {
    int tr_on[MAX_COMP] = {0};
    int plus_root[MAX_COMP * 3], minus_root[MAX_COMP * 3];
    int n_plus = 0, n_minus = 0;
    for (int i = 0; i < comp_count; i++) {
      if (comps[i].type == COMP_BATTERY || comps[i].type == COMP_ACSOURCE) {
        plus_root[n_plus++] = find_root(node_of(i, 0));
        minus_root[n_minus++] = find_root(node_of(i, 1));
      } else if (comps[i].type == COMP_GROUND) {
        minus_root[n_minus++] = find_root(node_of(i, 0));
      }
    }
    for (int i = 0; i < comp_count; i++) {
      if (comps[i].type != COMP_TRANSISTOR)
        continue;
      int rb = find_root(node_of(i, 2));
      int re = find_root(node_of(i, 1));
      int bp = 0, em = 0;
      for (int k = 0; k < n_plus; k++)
        if (plus_root[k] == rb)
          bp = 1;
      for (int k = 0; k < n_minus; k++)
        if (minus_root[k] == re)
          em = 1;
      if (bp && em)
        tr_on[i] = 1;
    }
    for (int i = 0; i < comp_count; i++)
      if (tr_on[i]) {
        cond_[i] = 1;
        unite(node_of(i, 0), node_of(i, 1));
      }
  }

  /* ---- Bangun node & V-fixed ---- */
    na_build_nodes();
  double *vfix = HeapAlloc(GetProcessHeap(), 0, sizeof(double) * NA_MAX);
  if (!vfix) return;

  /* Iterative Zener breakdown detection. */
  int has_zener = 0;
  for (int i = 0; i < comp_count; i++)
    if (comps[i].type == COMP_ZENER) { has_zener = 1; break; }

  if (has_zener) {
    for (int iter = 0; iter < 20; iter++) {
      na_assign_fixed_mode(vfix, SRC_MODE_NORMAL, -1);
      na_build(vfix, -1, -1);
      if (!na_solve()) break;

      int any_change = 0;
      for (int i = 0; i < comp_count; i++) {
        if (comps[i].type != COMP_ZENER) continue;
        if (dfwd_[i]) continue;                            /* sudah forward */
        int i0 = root_to_idx_[find_root(node_of(i, 0))];   /* A */
        int i1 = root_to_idx_[find_root(node_of(i, 1))];   /* K */
        if (i0 < 0 || i1 < 0 || i0 == i1) continue;

        double vrev = na_x[i1] - na_x[i0];                 /* V_K − V_A */
        double vz   = (comps[i].v > 0.0) ? comps[i].v : VZENER_DEFAULT;
        double tol  = 0.02;

        if (zener_bd_[i]) {
          if (vrev < vz - tol) { zener_bd_[i] = 0; any_change = 1; }
        } else {
          if (vrev > vz + tol) { zener_bd_[i] = 1; any_change = 1; }
        }
      }
      if (!any_change) break;
    }
  }

  /* Solve final dengan state breakdown stabil. */
  na_assign_fixed_mode(vfix, SRC_MODE_NORMAL, -1);

  /* ---- Solve global: V_thevenin di setiap node (semua LED OPEN) ---- */
  double Vth_node[NA_MAX];
  int vth_ok = 0;
  na_build(vfix, -1, -1);
  if (na_solve()) {
    for (int i = 0; i < na_n; i++)
      Vth_node[i] = na_x[i];
    vth_ok = 1;
  } else {
    for (int i = 0; i < na_n; i++)
      Vth_node[i] = 0.0;
  }

  /* ============================================================
     Pass 3: LED — hitung Vth, Rth, arus.
     ============================================================ */
  for (int i = 0; i < comp_count; i++) {
    if (comps[i].type != COMP_LED)
      continue;
    int r0 = find_root(node_of(i, 0)); /* anoda */
    int r1 = find_root(node_of(i, 1)); /* katoda */
    if (r0 == r1)
      continue; /* kedua pin shorted */

    int i0 = root_to_idx_[r0];
    int i1 = root_to_idx_[r1];
    if (i0 < 0 || i1 < 0)
      continue;

    double vth = vth_ok ? (Vth_node[i0] - Vth_node[i1]) : 0.0;

    /* --- Rth: semua sumber di-short, injeksi 1 A anoda→katoda --- */
    double vfix_rth[NA_MAX];
    for (int k = 0; k < na_n; k++)
      vfix_rth[k] = (vfix[k] != V_UNKNOWN) ? 0.0 : V_UNKNOWN;
    na_build(vfix_rth, i0, i1);
    double rth = -1.0;
    if (na_solve()) {
      rth = na_x[i0] - na_x[i1];
      if (rth < 0.0)
        rth = -rth; /* ambil nilai absolut */
    }
    /* Kalau anoda V-fixed (short ke sumber), Rth = 0. */
    if (vfix[i0] != V_UNKNOWN && vfix[i1] == V_UNKNOWN) {
      rth = 0.0;
    }

    double vf = (comps[i].v > 0.0) ? comps[i].v : VF_LED_DEFAULT;

    /* Loop terbuka atau tidak ada jalur konduktif. */
    if (rth < 0.0 || rth > R_OPEN_THRESH) {
      ilead_[i] = -1.0;
      continue;
    }

    /* Loop tertutup tanpa R: arus pendek → MELEDAK. */
    if (rth <= R_SHORT_THRESH && vth > vf) {
      lit_[i] = 1;
      sim_led_count++;
      ilead_[i] = 1e9;
      vdrop_[i] = vf;
      blown_[i] = 1;
      sim_exploded = 1;
      if (sim_iled_max < 1e8) {
        sim_iled_max = 1e9;
        sim_rloop = rth;
        sim_vsrc = vth;
        sim_vdrop_max = vf;
      }
      continue;
    }

    /* Arus LED. */
    double iled = 0.0;
    if (vth > vf)
      iled = (vth - vf) / rth;

    if (iled <= 0.0) {
      ilead_[i] = 0.0;
      continue; /* LED tidak menyala (Vth <= Vf) */
    }

    lit_[i] = 1;
    sim_led_count++;
    ilead_[i] = iled;
    vdrop_[i] = vf;

    if (iled > sim_iled_max) {
      sim_iled_max = iled;
      sim_rloop = rth;
      sim_vsrc = vth;
      sim_vdrop_max = vf;
    } else if (sim_iled_max >= 1e8 && iled < 1e8) {
      sim_iled_max = iled;
      sim_rloop = rth;
      sim_vsrc = vth;
      sim_vdrop_max = vf;
    }

    /* Klasifikasi nasib LED. */
    const double tol = 1.0 + 1e-6;
    if (iled > 5.0 * VF_LED_MAX * tol ||
        (iled > VF_LED_MAX * tol && vth > V_EXPLODE)) {
      blown_[i] = 1;
      sim_exploded = 1;
    } else if (iled > VF_LED_MAX * tol) {
      blown_[i] = 1;
      sim_overcur = 1;
    }
  }

  /* ============================================================
     Pass 4: Buzzer — ON bila |V(pin0) − V(pin1)| > 1 V.
     ============================================================ */
  for (int i = 0; i < comp_count; i++) {
    if (comps[i].type != COMP_BUZZER)
      continue;
    int i0 = root_to_idx_[find_root(node_of(i, 0))];
    int i1 = root_to_idx_[find_root(node_of(i, 1))];
    if (i0 < 0 || i1 < 0 || i0 == i1)
      continue;
    if (!vth_ok)
      continue;
    double dv = Vth_node[i0] - Vth_node[i1];
    if (dv < 0.0)
      dv = -dv;
    if (dv > 1.0)
      buzz_[i] = 1;
  }
  /* Buzzer yang seri dengan LED yang menyala ikut bunyi. */
  for (int i = 0; i < comp_count; i++) {
    if (comps[i].type != COMP_BUZZER || buzz_[i])
      continue;
    int rb0 = find_root(node_of(i, 0));
    int rb1 = find_root(node_of(i, 1));
    for (int j = 0; j < comp_count; j++) {
      if (comps[j].type != COMP_LED || !lit_[j])
        continue;
      int rl0 = find_root(node_of(j, 0));
      int rl1 = find_root(node_of(j, 1));
      if (rb0 == rl0 || rb0 == rl1 || rb1 == rl0 || rb1 == rl1) {
        buzz_[i] = 1;
        break;
      }
    }
  }

  /* ============================================================
     Pass 5: Voltmeter — bacaan = V(pin0) − V(pin1).
     ============================================================ */
  for (int i = 0; i < comp_count; i++) {
    if (comps[i].type != COMP_VOLTMETER)
      continue;
    int i0 = root_to_idx_[find_root(node_of(i, 0))];
    int i1 = root_to_idx_[find_root(node_of(i, 1))];
    if (i0 < 0 || i1 < 0)
      continue;
    if (!vth_ok)
      continue;
    double v = Vth_node[i0] - Vth_node[i1];
    /* Cek "floating": kalau V sangat besar (regularisasi), anggap tak
     * terhubung. */
    if (fabs(v) > 1e6)
      continue;
    vmconn_[i] = 1;
    vmread_[i] = v;
  }

  /* ============================================================
     Pass 6: Ammeter — bacaan = |V(pin0) − V(pin1)| / R_ammeter.
     ============================================================ */
  for (int i = 0; i < comp_count; i++){
    if(comps[i].type != COMP_AMMETER) continue;
    int i0 = root_to_idx_[find_root(node_of(i, 0))];
    int i1 = root_to_idx_[find_root(node_of(i, 1))];
    if(i0 < 0 || i1 < 0) continue;
    if(!vth_ok) continue;

    double dv = Vth_node[i0] - Vth_node[i1];
    if(fabs(dv) > 1e6) continue; /* floating */
    double current = fabs(dv) / 0.001; /* R_ammeter = 1 mOhm */
    amread_[i] = current;
  }

  /* ============================================================
     Hot-wire: kabel di net (+) → merah.
     ============================================================ */
  for (int i = 0; i < wire_count; i++) {
    Component *ca = find_comp_by_id(wires[i].a.comp_id);
    if (!ca)
      continue;
    int ia = (int)(ca - comps);
    if (ia < 0 || ia >= comp_count)
      continue;
    int r = find_root(node_of(ia, wires[i].a.pin_idx));
    for (int k = 0; k < comp_count; k++) {
      if (comps[k].type != COMP_BATTERY && comps[k].type != COMP_ACSOURCE)
        continue;
      if (find_root(node_of(k, 0)) == r) {
        hot_[i] = 1;
        break;
      }
    }
  }

  HeapFree(GetProcessHeap(), 0, vfix);
}

/* ============================================================================
   Bagian 4: API publik (kontrak schematic.h)
============================================================================ */
void sim_set_running(int on) {
  sim_running = on ? 1 : 0;
  dragging_id = -1;
  wiring_active = 0;
  sim_recompute();
  if (sim_running) {
    if (!sim_has_battery && !sim_has_ac)
      set_status(L"RUN: pasang Baterai atau Sumber AC dulu, "
                 L"lalu sambungkan kabel.");
    else
      set_status(L"RUN: %ls", sim_message());
  } else {
    set_status(L"Mode Edit. Klik ikon di toolbar, lalu klik canvas.");
  }
}

int sim_wire_hot(int idx) {
  if (idx < 0 || idx >= wire_count)
    return 0;
  return hot_[idx];
}

int sim_comp_lit(const Component *c) {
  if (!c)
    return 0;
  int i = (int)(c - comps);
  if (i < 0 || i >= comp_count)
    return 0;
  return lit_[i];
}

int sim_diode_fwd(const Component *c) {
  if (!c || c->type != COMP_DIODE)
    return 0;
  int i = (int)(c - comps);
  if (i < 0 || i >= comp_count)
    return 0;
  return dfwd_[i];
}

int sim_comp_conducting(const Component *c) {
  if (!c || c->type != COMP_TRANSISTOR)
    return 0;
  int i = (int)(c - comps);
  if (i < 0 || i >= comp_count)
    return 0;
  return cond_[i];
}

int sim_buzzer_on(const Component *c) {
  if (!c || c->type != COMP_BUZZER)
    return 0;
  int i = (int)(c - comps);
  if (i < 0 || i >= comp_count)
    return 0;
  if (!sim_running)
    return 0;
  return buzz_[i];
}

double sim_ldr_effective_r(const Component *c) {
  if (!c || c->type != COMP_LDR)
    return -1.0;
  double rdark = (c->v > 0.0) ? c->v : LDR_DARK_DEFAULT;
  return c->on ? rdark * LDR_LIGHT_RATIO : rdark;
}

double sim_voltmeter_reading(const Component *c) {
  if (!c || c->type != COMP_VOLTMETER)
    return -1.0;
  int i = (int)(c - comps);
  if (i < 0 || i >= comp_count)
    return -1.0;
  if (!vmconn_[i])
    return -1.0;
  return vmread_[i];
}

int sim_voltmeter_connected(const Component *c) {
  if (!c || c->type != COMP_VOLTMETER)
    return 0;
  int i = (int)(c - comps);
  if (i < 0 || i >= comp_count)
    return 0;
  return vmconn_[i];
}

void sim_format_si(double v, const wchar_t *unit, wchar_t *out, int cap) {
  const wchar_t *pre = L"";
  double a = fabs(v);
  if (a >= 1e3) {
    v /= 1e3;
    pre = L"k";
  } else if (a >= 1.0) { /* tetap */
  } else if (a >= 1e-3) {
    v /= 1e-3;
    pre = L"m";
  } else if (a >= 1e-6) {
    v /= 1e-6;
    pre = L"u";
  } else if (a >= 1e-9) {
    v /= 1e-9;
    pre = L"n";
  } else {
    v /= 1e-12;
    pre = L"p";
  }
  swprintf(out, cap, L"%.3g %ls%s", v, pre, unit ? unit : L"");
}

int sim_comp_blown(const Component *c) {
  if (!c)
    return 0;
  int i = (int)(c - comps);
  if (i < 0 || i >= comp_count)
    return 0;
  return blown_[i];
}

double sim_comp_current(const Component *c) {
  if (!c)
    return -1.0;
  int i = (int)(c - comps);
  if (i < 0 || i >= comp_count)
    return -1.0;
  return ilead_[i];
}

static wchar_t g_msg[256];
const wchar_t *sim_message(void) {
  wchar_t vi[32], vr[32], vd[32];
  if (!sim_has_battery && !sim_has_ac)
    return L"Belum ada sumber (baterai/AC) — LED tidak bisa menyala.";
  if (sim_short)
    return L"KORSLETING! (+) tersambung langsung ke (-).";
  if (sim_exploded) {
    sim_format_si(sim_iled_max, L"A", vi, 32);
    swprintf(g_msg, 256,
             L"BAHAYA! LED MELEDAK — arus loop %ls jauh melebihi batas "
             L"20 mA. Tambah/perbesar resistor seri!",
             vi);
    return g_msg;
  }
  if (sim_overcur) {
    sim_format_si(sim_iled_max, L"A", vi, 32);
    swprintf(g_msg, 256,
             L"PERINGATAN! LED rusak pelan — arus %ls > 20 mA (max). "
             L"Perbesar resistor seri.",
             vi);
    return g_msg;
  }
  if (sim_led_count > 0) {
    double dv = sim_vsrc - sim_vdrop_max;
    double ileff = (dv < 0.0) ? 0.0 : dv;
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
  return L"Rangkaian belum lengkap — LED mati. "
         L"Tutup saklar & sambungkan kabel.";
}

double sim_ammeter_reading(const Component *c){
  if(!c || c->type != COMP_AMMETER) return -1.0;
  int i = (int)(c-comps);
  if(i < 0 || i >= comp_count) return -1.0;
  return amread_[i];

}


/* Ammeter dianggap "terhubung" bila kedua pin berada di net berbeda dan
   simulasi nodal berhasil (amread_[i] >= 0). Konsisten dengan perilaku
   sim_voltmeter_connected(): pin mengambang / short → 0. */
int sim_ammeter_connected(const Component *c) {
    if (!c || c->type != COMP_AMMETER) return 0;
    int i = (int)(c - comps);
    if (i < 0 || i >= comp_count) return 0;
    return (amread_[i] >= 0.0);
}
int sim_zener_fwd(const Component *c) {
    if (!c || c->type != COMP_ZENER) return 0;
    int i = (int)(c - comps);
    if (i < 0 || i >= comp_count) return 0;
    return dfwd_[i];
}

int sim_zener_breakdown(const Component *c) {
    if (!c || c->type != COMP_ZENER) return 0;
    int i = (int)(c - comps);
    if (i < 0 || i >= comp_count) return 0;
    return zener_bd_[i];
}
/* ============================================================================
   Probe Osiloskop — domain-waktu via superposisi
============================================================================ */

/* Reset flag respon probe; dipanggil dari sim_recompute. */
int _sim_probe_resp_ready = 0;

double sim_ac_voltage(const Component *src, double t) {
    if (!src || src->type != COMP_ACSOURCE) return 0.0;
    double vrms = (src->v > 0.0) ? src->v : 220.0;
    double vpk  = vrms * M_SQRT2;
    double phase = 2.0 * M_PI * AC_FREQ_HZ * t;

    /* Fase ternormalisasi [0,1). */
    double p = phase / (2.0 * M_PI);
    p = p - floor(p);

    switch (src->wave) {
        case WAVE_SQUARE:
            return (p < 0.5) ? vpk : -vpk;
        case WAVE_TRIANGLE:
            if (p < 0.25)      return vpk * (4.0 * p);
            else if (p < 0.75) return vpk * (2.0 - 4.0 * p);
            else               return vpk * (4.0 * p - 4.0);
        case WAVE_SINE:
        default:
            return vpk * sin(phase);
    }
}

typedef struct {
    double dc;
    int    n_ac;
    int    ac_idx[MAX_COMP];
    double gain[MAX_COMP];
} ProbeResp;

static ProbeResp g_probe_resp[MAX_COMP];

void sim_build_probe_responses(void) {
    _sim_probe_resp_ready = 0;

    /* Kumpulkan indeks sumber AC. */
    int ac_idx[MAX_COMP], n_ac = 0;
    for (int i = 0; i < comp_count && n_ac < MAX_COMP; i++)
        if (comps[i].type == COMP_ACSOURCE) ac_idx[n_ac++] = i;

    double *vfix = (double *)HeapAlloc(GetProcessHeap(), 0, sizeof(double) * NA_MAX);
    if (!vfix) return;

    for (int p = 0; p < comp_count; p++) {
        if (comps[p].type != COMP_OSCPROBE) continue;
        ProbeResp *r = &g_probe_resp[p];
        r->dc = 0.0;
        r->n_ac = n_ac;
        for (int k = 0; k < n_ac; k++) {
            r->ac_idx[k] = ac_idx[k];
            r->gain[k]   = 0.0;
        }

        /* DC: semua sumber AC = 0. */
        na_assign_fixed_mode(vfix, SRC_MODE_DC_ONLY, -1);
        na_build(vfix, -1, -1);
        if (na_solve()) {
            int i0 = root_to_idx_[find_root(node_of(p, 0))];
            int i1 = root_to_idx_[find_root(node_of(p, 1))];
            if (i0 >= 0 && i1 >= 0) r->dc = na_x[i0] - na_x[i1];
        }

        /* Gain per sumber AC: satu sumber = 1 V, sisanya = 0. */
        for (int k = 0; k < n_ac; k++) {
            na_assign_fixed_mode(vfix, SRC_MODE_UNIT_ONE, ac_idx[k]);
            na_build(vfix, -1, -1);
            if (na_solve()) {
                int i0 = root_to_idx_[find_root(node_of(p, 0))];
                int i1 = root_to_idx_[find_root(node_of(p, 1))];
                if (i0 >= 0 && i1 >= 0) r->gain[k] = na_x[i0] - na_x[i1];
            }
        }
    }
    HeapFree(GetProcessHeap(), 0, vfix);
    _sim_probe_resp_ready = 1;
}

double sim_probe_voltage(const Component *probe, double t) {
    if (!probe || probe->type != COMP_OSCPROBE) return 0.0;
    int p = (int)(probe - comps);
    if (p < 0 || p >= comp_count) return 0.0;
    if (!_sim_probe_resp_ready) sim_build_probe_responses();

    ProbeResp *r = &g_probe_resp[p];
    double v = r->dc;
    for (int k = 0; k < r->n_ac; k++)
        v += r->gain[k] * sim_ac_voltage(&comps[r->ac_idx[k]], t);
    return v;
}