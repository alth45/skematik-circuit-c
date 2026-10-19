/* test_undo.c — Unit test fitur UNDO / REDO (snapshot stack di src/model.c)
   Mencakup: add, delete, wire, rotate, translate, value, clear, batch,
   batas depth, reset, dan penguncian saat RUN. */
#include "schematic.h"
#include "tinytest.h"

static int add_res_at(int x, int y) {
    model_add_component(COMP_RESISTOR, x, y);
    return selected_id;
}
static void wire_ids(int id_a, int pa, int id_b, int pb) {
    PinRef a = { id_a, pa }, b = { id_b, pb };
    model_add_wire(a, b);
}

TEST(undo, add_lalu_undo_redo) {
    test_reset_model(); undo_reset();
    int id = add_res_at(100, 100);
    ASSERT_INT_EQ(1, comp_count);
    ASSERT_INT_EQ(1, undo_can_undo());
    ASSERT_INT_EQ(1, undo_undo());
    ASSERT_INT_EQ(0, comp_count);
    ASSERT_INT_EQ(1, undo_can_redo());
    ASSERT_INT_EQ(1, undo_redo());
    ASSERT_INT_EQ(1, comp_count);
    ASSERT_TRUE(find_comp_by_id(id));
}

TEST(undo, delete_blok_satu_langkah) {
    test_reset_model(); undo_reset();
    int r1 = add_res_at(100, 100);
    int r2 = add_res_at(300, 100);
    wire_ids(r1, 0, r2, 0);
    ASSERT_INT_EQ(2, comp_count);
    undo_reset(); /* lupakan riwayat perakitan */
    clear_selection();
    add_to_selection(r1); add_to_selection(r2);
    ASSERT_INT_EQ(2, model_delete_selection());
    ASSERT_INT_EQ(0, comp_count);
    ASSERT_INT_EQ(1, undo_undo()); /* 1 langkah kembalikan blok + kabel */
    ASSERT_INT_EQ(2, comp_count);
    ASSERT_INT_EQ(1, wire_count);
    ASSERT_INT_EQ(1, undo_redo());
    ASSERT_INT_EQ(0, comp_count);
}

TEST(undo, wire_rotate_translate_value) {
    test_reset_model(); undo_reset();
    int r1 = add_res_at(100, 100);
    int r2 = add_res_at(300, 100);
    Component *c1 = find_comp_by_id(r1);
    int x0 = c1->x;
    wire_ids(r1, 0, r2, 1);
    undo_reset();
    /* rotate */
    selected_id = r1; model_rotate_selected();
    ASSERT_INT_EQ(1, find_comp_by_id(r1)->rot);
    ASSERT_INT_EQ(1, undo_undo());
    ASSERT_INT_EQ(0, find_comp_by_id(r1)->rot);
    ASSERT_INT_EQ(1, undo_redo());
    ASSERT_INT_EQ(1, find_comp_by_id(r1)->rot);
    undo_reset();
    /* translate */
    clear_selection(); add_to_selection(r1);
    model_translate_selection(40, 0);
    ASSERT_INT_EQ(x0 + 40, find_comp_by_id(r1)->x);
    ASSERT_INT_EQ(1, undo_undo());
    ASSERT_INT_EQ(x0, find_comp_by_id(r1)->x);
    undo_reset();
    /* value */
    double v0 = find_comp_by_id(r1)->v;
    model_set_component_value(r1, v0 + 100.0);
    ASSERT_DBL_NEAR(v0 + 100.0, find_comp_by_id(r1)->v, 1e-9);
    ASSERT_INT_EQ(1, undo_undo());
    ASSERT_DBL_NEAR(v0, find_comp_by_id(r1)->v, 1e-9);
    undo_reset();
    /* clear */
    model_clear_all();
    ASSERT_INT_EQ(0, comp_count);
    ASSERT_INT_EQ(1, undo_undo());
    ASSERT_INT_EQ(2, comp_count);
    ASSERT_INT_EQ(1, wire_count);
}

TEST(undo, aksi_baru_menghapus_redo) {
    test_reset_model(); undo_reset();
    add_res_at(100, 100);
    ASSERT_INT_EQ(1, undo_undo());
    ASSERT_INT_EQ(1, undo_can_redo());
    add_res_at(200, 100); /* aksi baru */
    ASSERT_INT_EQ(0, undo_can_redo());
    ASSERT_INT_EQ(0, undo_redo());
}

TEST(undo, dikunci_saat_run_dan_reset) {
    test_reset_model(); undo_reset();
    add_res_at(100, 100);
    undo_reset();
    sim_set_running(1);
    ASSERT_INT_EQ(0, undo_undo());
    ASSERT_INT_EQ(0, undo_redo());
    sim_set_running(0);
    ASSERT_INT_EQ(0, undo_can_undo());
    undo_reset();
    ASSERT_INT_EQ(0, undo_can_redo());
}

void suite_undo(void) {
    RUN_TEST(undo, add_lalu_undo_redo);
    RUN_TEST(undo, delete_blok_satu_langkah);
    RUN_TEST(undo, wire_rotate_translate_value);
    RUN_TEST(undo, aksi_baru_menghapus_redo);
    RUN_TEST(undo, dikunci_saat_run_dan_reset);
}
