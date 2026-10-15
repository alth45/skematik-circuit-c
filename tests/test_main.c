/* test_main.c — Entry point gabungan seluruh suite unit test.
   Dipanggil dengan: gcc ... tests/test_main.c tests/test_model.c
   tests/test_sim.c tests/test_selection.c tests/unit_stubs.c src/model.c src/sim.c */

#include <windows.h>
#include "tinytest.h"

void suite_model(void);
void suite_sim(void);
void suite_selection(void);

int wmain(void) {
    suite_model();
    suite_sim();
    suite_selection();
    return TEST_SUMMARY();
}
