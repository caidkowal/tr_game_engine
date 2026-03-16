#include <check.h>
#include <stdlib.h>
#include "player.h"


/* ============================================================
 * Setup and Teardown fixtures
 * ============================================================ */

static Player *p = NULL;

static void setup_player(void)
{


    ck_assert_int_eq(player_create(1,2,3,&p), OK);

}

static void teardown_player(void)
{
    player_destroy(p);
    p = NULL;
}

/* ============================================================
 * player_get_room successfully returns the room
 * ============================================================ */
START_TEST(test_get_room)
{
    p->room_id = 19;
    ck_assert_int_eq(player_get_room(p),19);    
}
END_TEST

/* ============================================================
 * player_get_room successfully returns -1
 * ============================================================ */
START_TEST(test_get_room_null)
{
    ck_assert_int_eq(player_get_room(NULL),-1);    
}
END_TEST

/* ============================================================
 *  player_get_pos successfully gets the player postion and returns OK
 * ============================================================ */
START_TEST(test_get_position)
{
    int x_out;
    int y_out;
    p->x = 12;
    p->y = 13; 
    ck_assert_int_eq(player_get_position(p, &x_out, &y_out), OK);
    ck_assert_int_eq(x_out, 12);
    ck_assert_int_eq(y_out, 13);
}
END_TEST

/* ============================================================
 *  player_get_pos room successfully returns INVALID_ARGUMENT
 * ============================================================ */
START_TEST(test_get_position_null)
{
    int y_out;
    p->x = 12;
    p->y = 13; 

    ck_assert_int_eq(player_get_position(p, NULL, &y_out), INVALID_ARGUMENT);

}
END_TEST

/* ============================================================
 *  player_set_pos successfully sets position returns OK
 * ============================================================ */
START_TEST(test_set_position)
{

    int x = 9;
    int y = 10;

    ck_assert_int_eq(player_set_position(p, x, y), OK);
    ck_assert_int_eq(p->x, 9);
    ck_assert_int_eq(p->y, 10);

}
END_TEST

/* ============================================================
 *  player_set_pos successfully returns NULL
 * ============================================================ */
START_TEST(test_set_position_null)
{

    int x = 9;
    int y = 10;

    ck_assert_int_eq(player_set_position(NULL, x, y), INVALID_ARGUMENT);

}
END_TEST

/* ============================================================
 *  player_move_to_room successfully returns OK
 * ============================================================ */
START_TEST(test_player_move_to_room)
{

    int new_room_id = 57;

    ck_assert_int_eq(player_move_to_room(p,new_room_id), OK);

    ck_assert_int_eq(p->room_id, new_room_id);

}
END_TEST



START_TEST(test_player_reset)
{



    int starting_room_id = 123;
    int start_x = 4;
    int start_y = 5;

    ck_assert_int_eq(player_reset_to_start(p,starting_room_id, start_x, start_y), OK);

    ck_assert_int_eq(p->room_id, starting_room_id);
    ck_assert_int_eq(p->x, start_x);
    ck_assert_int_eq(p->y, start_y);

}
END_TEST

/* ============================================================
 * Treasure Collection Tests
 * ============================================================ */

START_TEST(test_player_try_collect_basic)
{
    //create a treasure
    Treasure treasure;
    treasure.id = 100;
    treasure.name = strdup("Gold Coin");
    treasure.x = 5;
    treasure.y = 6;
    treasure.collected = false;
    
    //initially no treasures collected
    ck_assert_int_eq(p->collected_count, 0);
    ck_assert_ptr_null(p->collected_treasures);
    
    //collect the treasure
    Status s = player_try_collect(p, &treasure);
    
    //should succeed
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(p->collected_count, 1);
    ck_assert_ptr_nonnull(p->collected_treasures);
    ck_assert_ptr_eq(p->collected_treasures[0], &treasure);
    ck_assert(treasure.collected == true);
    
    free(treasure.name);
}
END_TEST

START_TEST(test_player_try_collect_null_player)
{
    Treasure treasure;
    treasure.collected = false;
    
    //try to collect with null player
    Status s = player_try_collect(NULL, &treasure);
    
    ck_assert_int_eq(s, NULL_POINTER);
}
END_TEST

START_TEST(test_player_try_collect_null_treasure)
{
    //try to collect null treasure
    Status s = player_try_collect(p, NULL);
    
    ck_assert_int_eq(s, NULL_POINTER);
}
END_TEST

START_TEST(test_player_try_collect_already_collected)
{
    Treasure treasure;
    treasure.id = 101;
    treasure.name = strdup("Silver Ring");
    treasure.collected = false;
    
    //collect once
    Status s1 = player_try_collect(p, &treasure);
    ck_assert_int_eq(s1, OK);
    ck_assert(treasure.collected == true);
    
    //try to collect again - should fail
    Status s2 = player_try_collect(p, &treasure);
    ck_assert_int_eq(s2, INVALID_ARGUMENT);
    ck_assert_int_eq(p->collected_count, 1);
    
    free(treasure.name);
}
END_TEST

START_TEST(test_player_try_collect_multiple)
{
    Treasure t1, t2, t3;
    
    t1.id = 1;
    t1.name = strdup("Treasure 1");
    t1.collected = false;
    
    t2.id = 2;
    t2.name = strdup("Treasure 2");
    t2.collected = false;
    
    t3.id = 3;
    t3.name = strdup("Treasure 3");
    t3.collected = false;
    
    //collect all three
    ck_assert_int_eq(player_try_collect(p, &t1), OK);
    ck_assert_int_eq(player_try_collect(p, &t2), OK);
    ck_assert_int_eq(player_try_collect(p, &t3), OK);
    
    //verify count and array contents
    ck_assert_int_eq(p->collected_count, 3);
    ck_assert_ptr_eq(p->collected_treasures[0], &t1);
    ck_assert_ptr_eq(p->collected_treasures[1], &t2);
    ck_assert_ptr_eq(p->collected_treasures[2], &t3);
    ck_assert(t1.collected == true);
    ck_assert(t2.collected == true);
    ck_assert(t3.collected == true);
    
    free(t1.name);
    free(t2.name);
    free(t3.name);
}
END_TEST

START_TEST(test_player_has_collected_treasure_true)
{
    Treasure treasure;
    treasure.id = 42;
    treasure.name = strdup("Magic Sword");
    treasure.collected = false;
    
    //collect it
    player_try_collect(p, &treasure);
    
    //check if player has it
    bool has_it = player_has_collected_treasure(p, 42);
    ck_assert(has_it == true);
    
    free(treasure.name);
}
END_TEST

START_TEST(test_player_has_collected_treasure_false)
{
    Treasure treasure;
    treasure.id = 42;
    treasure.name = strdup("Magic Sword");
    treasure.collected = false;
    
    //collect it
    player_try_collect(p, &treasure);
    
    //check for different id - should return false
    bool has_it = player_has_collected_treasure(p, 999);
    ck_assert(has_it == false);
    
    free(treasure.name);
}
END_TEST

START_TEST(test_player_has_collected_treasure_null)
{
    //null player should return false
    bool has_it = player_has_collected_treasure(NULL, 42);
    ck_assert(has_it == false);
}
END_TEST

START_TEST(test_player_has_collected_treasure_negative_id)
{
    //negative id should return false
    bool has_it = player_has_collected_treasure(p, -1);
    ck_assert(has_it == false);
}
END_TEST

START_TEST(test_player_get_collected_count_zero)
{
    //no treasures collected yet
    int count = player_get_collected_count(p);
    ck_assert_int_eq(count, 0);
}
END_TEST

START_TEST(test_player_get_collected_count_multiple)
{
    Treasure t1, t2;
    
    t1.id = 1;
    t1.name = strdup("T1");
    t1.collected = false;
    
    t2.id = 2;
    t2.name = strdup("T2");
    t2.collected = false;
    
    //collect two treasures
    player_try_collect(p, &t1);
    player_try_collect(p, &t2);
    
    int count = player_get_collected_count(p);
    ck_assert_int_eq(count, 2);
    
    free(t1.name);
    free(t2.name);
}
END_TEST

START_TEST(test_player_get_collected_count_null)
{
    //null player should return 0
    int count = player_get_collected_count(NULL);
    ck_assert_int_eq(count, 0);
}
END_TEST

START_TEST(test_player_get_collected_treasures_basic)
{
    Treasure t1, t2;
    
    t1.id = 10;
    t1.name = strdup("T1");
    t1.collected = false;
    
    t2.id = 20;
    t2.name = strdup("T2");
    t2.collected = false;
    
    //collect two treasures
    player_try_collect(p, &t1);
    player_try_collect(p, &t2);
    
    //get the array
    int count = 0;
    const Treasure * const *treasures = player_get_collected_treasures(p, &count);
    
    //verify array and count
    ck_assert_ptr_nonnull(treasures);
    ck_assert_int_eq(count, 2);
    ck_assert_ptr_eq(treasures[0], &t1);
    ck_assert_ptr_eq(treasures[1], &t2);
    ck_assert_int_eq(treasures[0]->id, 10);
    ck_assert_int_eq(treasures[1]->id, 20);
    
    free(t1.name);
    free(t2.name);
}
END_TEST

START_TEST(test_player_get_collected_treasures_null_player)
{
    int count = 0;
    
    //null player should return null
    const Treasure * const *treasures = player_get_collected_treasures(NULL, &count);
    ck_assert_ptr_null(treasures);
}
END_TEST

START_TEST(test_player_get_collected_treasures_null_count)
{
    //null count_out should return null
    const Treasure * const *treasures = player_get_collected_treasures(p, NULL);
    ck_assert_ptr_null(treasures);
}
END_TEST

START_TEST(test_player_reset_clears_treasures)
{
    Treasure t1, t2;
    
    t1.id = 1;
    t1.name = strdup("T1");
    t1.collected = false;
    
    t2.id = 2;
    t2.name = strdup("T2");
    t2.collected = false;
    
    //collect two treasures
    player_try_collect(p, &t1);
    player_try_collect(p, &t2);
    ck_assert_int_eq(p->collected_count, 2);
    
    //reset player
    player_reset_to_start(p, 1, 5, 5);
    
    //treasures should be cleared
    ck_assert_int_eq(p->collected_count, 0);
    ck_assert_ptr_null(p->collected_treasures);
    
    free(t1.name);
    free(t2.name);
}
END_TEST

/* ============================================================
 * Suite Creation Function
 * 
 * This function builds and returns a test suite for the Check framework.
 * A Suite is a collection of test cases that are run together.
 * ============================================================ */

Suite *player_suite(void)
{
    // Create a new test suite with a descriptive name
    Suite *s = suite_create("Player");
    
    // Create a test case to group related tests
    TCase *tc = tcase_create("HappyPath");
    TCase *a2 = tcase_create("Assignment2");

    // Attach setup and teardown functions to run before/after each test
    tcase_add_checked_fixture(tc, setup_player, teardown_player);

    tcase_add_test(tc, test_get_room);
    tcase_add_test(tc, test_get_room_null);
    tcase_add_test(tc, test_get_position);
    tcase_add_test(tc, test_get_position_null);
    tcase_add_test(tc, test_set_position);

    tcase_add_test(tc, test_set_position_null);
    tcase_add_test(tc, test_player_move_to_room);
    tcase_add_test(tc, test_player_reset);

    //assignment 2 tests
    tcase_add_checked_fixture(a2, setup_player, teardown_player);

    tcase_add_test(a2, test_player_try_collect_basic);
    tcase_add_test(a2, test_player_try_collect_null_player);
    tcase_add_test(a2, test_player_try_collect_null_treasure);
    tcase_add_test(a2, test_player_try_collect_already_collected);
    tcase_add_test(a2, test_player_try_collect_multiple);
    
    tcase_add_test(a2, test_player_has_collected_treasure_true);
    tcase_add_test(a2, test_player_has_collected_treasure_false);
    tcase_add_test(a2, test_player_has_collected_treasure_null);
    tcase_add_test(a2, test_player_has_collected_treasure_negative_id);
    
    tcase_add_test(a2, test_player_get_collected_count_zero);
    tcase_add_test(a2, test_player_get_collected_count_multiple);
    tcase_add_test(a2, test_player_get_collected_count_null);
    
    tcase_add_test(a2, test_player_get_collected_treasures_basic);
    tcase_add_test(a2, test_player_get_collected_treasures_null_player);
    tcase_add_test(a2, test_player_get_collected_treasures_null_count);
    
    tcase_add_test(a2, test_player_reset_clears_treasures);

    // Add the test case to the suite
    suite_add_tcase(s, tc);
    suite_add_tcase(s, a2);
    
    // Return the complete suite so main() can run it
    return s;
}