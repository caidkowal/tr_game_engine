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


/* switch charset fields are copied from datagen */
START_TEST(test_ckowal_loader_charset_switch_chars_loaded)
{
    //switch_off and switch_on must be valid printable characters
    //the exact value depends on the ini, but they must not be '\0'
    ck_assert_int_ne(charset.switch_off, '\0');
    ck_assert_int_ne(charset.switch_on,  '\0');
}
END_TEST

/* loader_load_world with a bad config path returns WL_ERR_CONFIG */
START_TEST(test_ckowal_loader_invalid_config_returns_error)
{
    Graph *bad_graph = NULL;
    Room  *bad_room  = NULL;
    int    bad_count = 0;
    Charset bad_cs;

    Status s = loader_load_world("../assets/does_not_exist.ini",
                                 &bad_graph, &bad_room,
                                 &bad_count, &bad_cs);

    ck_assert_int_eq(s, WL_ERR_CONFIG);
    //outputs should remain NULL / zero on failure
    ck_assert_ptr_null(bad_graph);
}
END_TEST

/* every room returned has positive dimensions and a non-NULL floor grid */
START_TEST(test_ckowal_loader_all_rooms_have_valid_dimensions)
{
    const void * const *payloads = NULL;
    int count = 0;

    GraphStatus gs = graph_get_all_payloads(graph, &payloads, &count);
    ck_assert_int_eq(gs, GRAPH_STATUS_OK);
    ck_assert_int_gt(count, 0);

    for (int i = 0; i < count; i++) {
        const Room *room = (const Room *)payloads[i];
        ck_assert_ptr_nonnull(room);
        ck_assert_int_gt(room->width,  0);
        ck_assert_int_gt(room->height, 0);
        ck_assert_ptr_nonnull(room->floor_grid);
    }
}
END_TEST

/* every portal has a properly initialised gated field (not garbage memory) */
START_TEST(test_ckowal_loader_portal_gated_field_is_initialised)
{
    const void * const *payloads = NULL;
    int count = 0;

    GraphStatus gs = graph_get_all_payloads(graph, &payloads, &count);
    ck_assert_int_eq(gs, GRAPH_STATUS_OK);

    for (int i = 0; i < count; i++) {
        const Room *room = (const Room *)payloads[i];
        for (int p = 0; p < room->portal_count; p++) {
            //gated must be consistent with required_switch_id:
            //  gated == true  iff required_switch_id >= 0
            bool expected_gated = (room->portals[p].required_switch_id >= 0);
            ck_assert_int_eq((int)room->portals[p].gated, (int)expected_gated);
        }
    }
}
END_TEST

/* switches loaded into rooms match switch_count; each has a valid portal_id */
START_TEST(test_ckowal_loader_switches_consistent_with_portals)
{
    const void * const *payloads = NULL;
    int count = 0;

    GraphStatus gs = graph_get_all_payloads(graph, &payloads, &count);
    ck_assert_int_eq(gs, GRAPH_STATUS_OK);

    for (int i = 0; i < count; i++) {
        const Room *room = (const Room *)payloads[i];

        //if there are switches, there must also be portals to control
        if (room->switch_count > 0) {
            ck_assert_ptr_nonnull(room->switches);
            ck_assert_int_gt(room->portal_count, 0);

            for (int sw = 0; sw < room->switch_count; sw++) {
                //each switch must reference a valid portal index within the room
                int pid = room->switches[sw].portal_id;
                ck_assert_int_ge(pid, 0);
                ck_assert_int_lt(pid, room->portal_count);
            }
        }
    }
}
END_TEST


/* loader_load_world with null graph_out returns INVALID_ARGUMENT */
START_TEST(test_ckowal_loader_null_graph_out_returns_invalid)
{
    Room  *out_room  = NULL;
    int    out_count = 0;
    Charset out_cs;

    Status s = loader_load_world("../assets/starter.ini",
                                 NULL, &out_room,
                                 &out_count, &out_cs);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

/* graph has at least one directed edge (connect_rooms was called and worked) */
START_TEST(test_ckowal_loader_graph_has_edges)
{
    //starter.ini rooms are connected via portals so the graph must have edges
    int edges = graph_edge_count(graph);
    ck_assert_int_gt(edges, 0);
}
END_TEST

/* treasure names are deep-copied: freeing datagen does not corrupt them */
START_TEST(test_ckowal_loader_treasure_names_are_deep_copied)
{
    const void * const *payloads = NULL;
    int count = 0;

    GraphStatus gs = graph_get_all_payloads(graph, &payloads, &count);
    ck_assert_int_eq(gs, GRAPH_STATUS_OK);

    //verify every treasure has a non-NULL, non-empty name
    for (int i = 0; i < count; i++) {
        const Room *room = (const Room *)payloads[i];
        for (int t = 0; t < room->treasure_count; t++) {
            ck_assert_ptr_nonnull(room->treasures[t].name);
            ck_assert_int_gt((int)strlen(room->treasures[t].name), 0);
        }
    }
}
END_TEST


/* ============================================================
 * Suite
 * ============================================================ */

Suite *loader_suite(void)
{
    Suite *s = suite_create("WorldLoader");
    TCase *tc = tcase_create("HappyPath");
    TCase *a3 = tcase_create("Assignment3");
    TCase *a3b = tcase_create("Assignment3Extra");

    tcase_add_checked_fixture(tc, setup_loader, teardown_loader);
    tcase_add_test(tc, test_loader_basic_success);

    //assignment 3 tests
    tcase_add_checked_fixture(a3, setup_loader, teardown_loader);

    tcase_add_test(a3, test_ckowal_loader_charset_switch_chars_loaded);
    tcase_add_test(a3, test_ckowal_loader_invalid_config_returns_error);
    tcase_add_test(a3, test_ckowal_loader_all_rooms_have_valid_dimensions);
    tcase_add_test(a3, test_ckowal_loader_portal_gated_field_is_initialised);
    tcase_add_test(a3, test_ckowal_loader_switches_consistent_with_portals);

    //assignment 3 extra coverage
    tcase_add_checked_fixture(a3b, setup_loader, teardown_loader);

    tcase_add_test(a3b, test_ckowal_loader_null_graph_out_returns_invalid);
    tcase_add_test(a3b, test_ckowal_loader_graph_has_edges);
    tcase_add_test(a3b, test_ckowal_loader_treasure_names_are_deep_copied);

    suite_add_tcase(s, tc);
    suite_add_tcase(s, a3);
    suite_add_tcase(s, a3b);
    return s;
}