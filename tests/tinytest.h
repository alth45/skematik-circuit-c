/* tinytest.h — kerangka unit-test super-minimal (tanpa dependensi eksternal)
   Dipakai oleh seluruh berkas tests/test_*.c pada proyek Schematic-C.

   Cara pakai:
     TEST(model, snap_membulat_ke_grid) { ASSERT_INT_EQ(100, snap(98)); }
     RUN(model, snap_membulat_ke_grid);

   Catatan Win32: output ditulis lewat WriteFile ke STD_OUTPUT_HANDLE supaya
   aman untuk teks UTF-8 sekaligus tetap bisa di-capture lewat pipe;
   fallback ke fwrite bila handle tidak valid. */
#ifndef TINYTEST_H
#define TINYTEST_H

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static int tt_pass = 0, tt_fail = 0, tt_checks = 0;
static int tt_cur_failed = 0;
static char tt_buf[512];

static void tt_emit(const char *s) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD n = (DWORD)strlen(s), wr = 0;
    if (h && h != INVALID_HANDLE_VALUE && WriteFile(h, s, n, &wr, NULL)) return;
    fputs(s, stdout);
    fflush(stdout);
}

/* Satu kegagalan per test: laporan pertama yang dicatat agar output ringkas. */
#define TT_FAIL_AT(msg)                                                      \
    do {                                                                     \
        if (!tt_cur_failed) {                                                \
            tt_cur_failed = 1;                                               \
            tt_fail++;                                                       \
            snprintf(tt_buf, sizeof tt_buf,                                  \
                     "  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, (msg));     \
            tt_emit(tt_buf);                                                 \
        }                                                                    \
    } while (0)

#define ASSERT_TRUE(expr)                                                    \
    do { tt_checks++;                                                        \
        if (!(expr)) { char _m[256]; snprintf(_m, sizeof _m,                 \
            "expected true: %s", #expr); TT_FAIL_AT(_m); } else tt_pass++;   \
    } while (0)

#define ASSERT_FALSE(expr)                                                   \
    do { tt_checks++;                                                        \
        if ((expr)) { char _m[256]; snprintf(_m, sizeof _m,                  \
            "expected false: %s", #expr); TT_FAIL_AT(_m); } else tt_pass++;  \
    } while (0)

#define ASSERT_INT_EQ(exp, got)                                              \
    do { tt_checks++; long _e = (long)(exp), _g = (long)(got);               \
        if (_e != _g) { char _m[256];                                        \
            snprintf(_m, sizeof _m, "expected %ld, got %ld  [%s]",          \
                     _e, _g, #got); TT_FAIL_AT(_m);                          \
        } else tt_pass++;                                                    \
    } while (0)

#define ASSERT_DBL_NEAR(exp, got, tol)                                       \
    do { tt_checks++; double _e = (double)(exp), _g = (double)(got),          \
             _t = (double)(tol);                                             \
        if (!(_g >= _e - _t && _g <= _e + _t)) { char _m[256];               \
            snprintf(_m, sizeof _m, "expected %.7g +/- %.7g, got %.7g  [%s]",\
                     _e, _t, _g, #got); TT_FAIL_AT(_m);                       \
        } else tt_pass++;                                                    \
    } while (0)

#define ASSERT_NOT_NULL(p)                                                   \
    do { tt_checks++;                                                        \
        if ((p) == NULL) { char _m[256]; snprintf(_m, sizeof _m,             \
            "expected non-NULL: %s", #p); TT_FAIL_AT(_m); } else tt_pass++;  \
    } while (0)

#define ASSERT_NULL(p)                                                       \
    do { tt_checks++;                                                        \
        if ((p) != NULL) { char _m[256]; snprintf(_m, sizeof _m,             \
            "expected NULL: %s", #p); TT_FAIL_AT(_m); } else tt_pass++;      \
    } while (0)

/* Cek substring (wide) pada pesan / status — dipakai menguji sim_message(). */
#define ASSERT_WSTR_HAS(hay, needle)                                         \
    do { tt_checks++;                                                        \
        if (!wcsstr((hay), (needle))) { char _m[448];                        \
            _m[0] = 0;                                                       \
            WideCharToMultiByte(CP_UTF8, 0, (hay), -1, _m, sizeof _m - 64, NULL, NULL); \
            strncat(_m, " <|not contains|> ", sizeof _m - strlen(_m) - 1);   \
            WideCharToMultiByte(CP_UTF8, 0, (needle), -1,                    \
                _m + strlen(_m), sizeof _m - strlen(_m) - 1, NULL, NULL);    \
            TT_FAIL_AT(_m);                                                  \
        } else tt_pass++;                                                    \
    } while (0)

#define TEST(suite, name) static void suite##_##name(void)
#define RUN(suite, name)                                                     \
    do {                                                                     \
        tt_cur_failed = 0;                                                   \
        tt_emit("  - " #name "\n");                                          \
        suite##_##name();                                                    \
    } while (0)

#define SUITE_BEGIN(title)                                                   \
    do { tt_emit("== "); tt_emit(title); tt_emit(" ==\n"); } while (0)

#define TEST_SUMMARY()                                                       \
    ({ int _rc = (tt_fail == 0) ? 0 : 1; char _m[256];                       \
       snprintf(_m, sizeof _m,                                               \
           "\nRingkasan: %d assertion, %d lulus, %d test GAGAL -> exit %d\n",\
           tt_checks, tt_pass, tt_fail, _rc);                                \
       tt_emit(_m); _rc; })

#endif /* TINYTEST_H */
