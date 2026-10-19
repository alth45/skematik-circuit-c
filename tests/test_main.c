/* test_main.c — Entry point gabungan seluruh suite unit test.
   Dibangun HANYA oleh run_tests.sh (mode -DSCHEMATIC_TEST_STUBS=1):
     tests/test_main.c tests/test_model.c tests/test_sim.c
     tests/test_selection.c tests/test_support.c src/model.c src/sim.c
   Build produksi (build.bat / build.sh) tidak pernah menyentuh folder
   tests/ — pemisahan dijaga #error di schematic.h + guard di skrip build. */

#include <windows.h>
#include "tinytest.h"

void suite_model(void);
void suite_sim(void);
void suite_selection(void);
void suite_undo(void);

int wmain(void) {
    suite_model();
    suite_sim();
    suite_selection();
    suite_undo();
    return TEST_SUMMARY();
}
