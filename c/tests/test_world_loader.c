#include <check.h>
#include <stdlib.h>

#include "world_loader.h"
#include "graph.h"
#include "room.h"


static Graph *graph = NULL;
static Room *first_room = NULL;
static int num_rooms = 0;
static Charset charset;

/* ============================================================
 * Setup / Teardown
 * ============================================================ */

static void setup_loader(void)
{
    Status s = loader_load_world("../assets/starter.ini",
                                 &graph,
                                 &first_room,
                                 &num_rooms,
                                 &charset);

    ck_assert_int_eq(s, OK);
}

static void teardown_loader(void)
{
    if (graph) {
        graph_destroy(graph);
        graph = NULL;
    }

    first_room = NULL;
    num_rooms = 0;
}


START_TEST(test_loader_basic_success)
{
    //loader should have already succeeded in setup
    ck_assert_ptr_nonnull(graph);
    ck_assert_ptr_nonnull(first_room);

    //starter config has 3 rooms
    ck_assert_int_eq(num_rooms, 3);
    ck_assert_int_eq(graph_size(graph), 3);

    //harset should be loaded correctly
    ck_assert_int_eq(charset.wall, '#');
    ck_assert_int_eq(charset.floor, '.');
    ck_assert_int_eq(charset.treasure, '$');
    ck_assert_int_eq(charset.portal, 'X');
    ck_assert_int_eq(charset.player, '@');

    //basic checks
    ck_assert_int_eq(first_room->id, 0);
    ck_assert_int_gt(first_room->width, 0);
    ck_assert_int_gt(first_room->height, 0);
    ck_assert_ptr_nonnull(first_room->floor_grid);
}
END_TEST

/* ============================================================
 * Suite
 * ============================================================ */

Suite *loader_suite(void)
{
    Suite *s = suite_create("WorldLoader");
    TCase *tc = tcase_create("HappyPath");

    tcase_add_checked_fixture(tc, setup_loader, teardown_loader);
    tcase_add_test(tc, test_loader_basic_success);

    suite_add_tcase(s, tc);
    return s;
}
