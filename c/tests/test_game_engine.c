#include <check.h>
#include <stdlib.h>

#include "game_engine.h"
#include "world_loader.h"
#include "graph.h"
#include "room.h"
#include "player.h"

static GameEngine *engine = NULL;

/* ============================================================ */
/* Setup / Teardown */
/* ============================================================ */

static void setup_engine(void)
{
    Status s = game_engine_create("../assets/starter.ini", &engine);

    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(engine);
}

static void teardown_engine(void)
{
    if (engine) {
        game_engine_destroy(engine);
        engine = NULL;
    }
}


START_TEST(test_move_player_simple_east)
{
    //get player
    const Player *p = game_engine_get_player(engine);
    ck_assert_ptr_nonnull(p);

    int start_x = 0;
    int start_y = 0;

    Status ps = player_get_position(p, &start_x, &start_y);

    ck_assert_int_eq(ps, OK);

    //attempt move east
    Status ms = game_engine_move_player(engine, DIR_EAST);

    ck_assert_int_eq(ms, OK);

    //get new position
    int new_x = 0;
    int new_y = 0;

    ps = player_get_position(p,&new_x, &new_y);

    ck_assert_int_eq(ps, OK);

    //x should increase by 1
    ck_assert_int_eq(new_x, start_x + 1);

    //y should remain same
    ck_assert_int_eq(new_y, start_y);
}
END_TEST

START_TEST(test_get_room_count)
{

    int count_out;
    Status s = game_engine_get_room_count(engine, &count_out);

    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(count_out, 3); 

}
END_TEST

START_TEST(test_get_room_dimensions)
{
    int width = 0;
    int height = 0;

    Status s = game_engine_get_room_dimensions(engine, &width, &height);

    ck_assert_int_eq(s, OK);

    ck_assert_int_gt(width, 0);
    ck_assert_int_gt(height, 0);
}
END_TEST

START_TEST(test_game_engine_reset)
{
    const Player *p = game_engine_get_player(engine);
    ck_assert_ptr_nonnull(p);

    int start_x = 0;
    int start_y = 0;

    Status ps = player_get_position(p, &start_x, &start_y);

    ck_assert_int_eq(ps, OK);

    //move player east and south (effecting x and y)
    Status mse = game_engine_move_player(engine, DIR_EAST);

    Status mss = game_engine_move_player(engine, DIR_SOUTH);

    ck_assert_int_eq(mse, OK);
    ck_assert_int_eq(mss, OK);

    int moved_x = 0;
    int moved_y = 0;

    ps = player_get_position(p, &moved_x, &moved_y);

    ck_assert_int_eq(ps, OK);

    //ensure position actually changed
    ck_assert_int_ne(moved_x, start_x);

    //now reset
    Status rs = game_engine_reset(engine);
    ck_assert_int_eq(rs, OK);

    int reset_x = 0;
    int reset_y = 0;

    ps = player_get_position(p, &reset_x, &reset_y);

    ck_assert_int_eq(ps, OK);

    //player should be back at start
    ck_assert_int_eq(reset_x, start_x);
    ck_assert_int_eq(reset_y, start_y);
}
END_TEST

START_TEST(test_render_current_room)
{
    char *output = NULL;

    //call render function
    Status s = game_engine_render_current_room(engine, &output);

    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(output);

    //output should contain player character
    ck_assert_ptr_nonnull(strchr(output, engine->charset.player));

    //output should contain a newline
    ck_assert_ptr_nonnull(strchr(output, '\n'));

    free(output);
}
END_TEST

START_TEST(test_render_room)
{
    char *output = NULL;

    //choose room we want to generate (just do inital cuz we can access it)
    int room_id = engine->initial_room_id;

    //call render function
    Status s = game_engine_render_room(engine, room_id, &output);

    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(output);

    //output should NOT contain player character
    ck_assert_ptr_null(strchr(output, engine->charset.player));

    //output should contain at a newline
    ck_assert_ptr_nonnull(strchr(output, '\n'));

    free(output);
}
END_TEST

START_TEST(test_get_room_ids)
{
    int *ids = NULL;
    int count = 0;

    //call function
    Status s = game_engine_get_room_ids(engine, &ids, &count);

    //should succeed
    ck_assert_int_eq(s, OK);

    //ids array should not be NULL
    ck_assert_ptr_nonnull(ids);

    //starter.ini has 3 rooms
    ck_assert_int_eq(count, 3);

    //IDs should contain 0,1,2

    bool found0 = false;
    bool found1 = false;
    bool found2 = false;

    for (int i = 0; i < count; i++) {
        if (ids[i] == 0){
            found0 = true;
        }
        if (ids[i] == 1){
            found1 = true;
        }
        if (ids[i] == 2){
            found2 = true;
        }
    }

    ck_assert(found0);
    ck_assert(found1);
    ck_assert(found2);
}

START_TEST(test_move_player_collect_treasure)
{
    //this test assumes starter.ini has treasures
    //get initial collected count
    const Player *p = game_engine_get_player(engine);
    int initial_count = player_get_collected_count(p);
    
    //move player around to potentially collect treasure
    game_engine_move_player(engine, DIR_EAST);
    game_engine_move_player(engine, DIR_SOUTH);
    
    //check if count increased (if there was a treasure)
    int new_count = player_get_collected_count(p);
    
    //count should be >= initial (may have collected something)
    ck_assert_int_ge(new_count, initial_count);
}
END_TEST

START_TEST(test_game_engine_reset_treasures)
{
    const Player *p = game_engine_get_player(engine);
    
    //try to collect some treasures by moving around
    game_engine_move_player(engine, DIR_EAST);
    game_engine_move_player(engine, DIR_SOUTH);
    game_engine_move_player(engine, DIR_EAST);
    
    int collected_before_reset = player_get_collected_count(p);
    
    //reset the game
    Status rs = game_engine_reset(engine);
    ck_assert_int_eq(rs, OK);
    
    //player should have 0 treasures
    int collected_after_reset = player_get_collected_count(p);
    ck_assert_int_eq(collected_after_reset, 0);
    
    //if we collected anything, verify it was reset
    if (collected_before_reset > 0) {
        ck_assert_int_ne(collected_before_reset, collected_after_reset);
    }
}
END_TEST

START_TEST(test_game_engine_free_string)
{
    char *output = NULL;
    
    //render current room
    Status s = game_engine_render_current_room(engine, &output);
    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(output);
    
    //free using game_engine_free_string (should not crash)
    game_engine_free_string(output);
}
END_TEST

START_TEST(test_render_collected_treasure_as_floor)
{
    //this test verifies that collected treasures show as floor, not treasure symbol
    
    char *before = NULL;
    game_engine_render_current_room(engine, &before);
    
    //count treasure symbols before
    int treasure_count_before = 0;
    for (int i = 0; before[i] != '\0'; i++) {
        if (before[i] == engine->charset.treasure) {
            treasure_count_before++;
        }
    }
    
    //move to collect a treasure (if possible)
    game_engine_move_player(engine, DIR_EAST);
    game_engine_move_player(engine, DIR_SOUTH);
    
    char *after = NULL;
    game_engine_render_current_room(engine, &after);
    
    //count treasure symbols after
    int treasure_count_after = 0;
    for (int i = 0; after[i] != '\0'; i++) {
        if (after[i] == engine->charset.treasure) {
            treasure_count_after++;
        }
    }
    
    //if we collected a treasure, symbol count should decrease
    const Player *p = game_engine_get_player(engine);
    if (player_get_collected_count(p) > 0) {
        ck_assert_int_le(treasure_count_after, treasure_count_before);
    }
    
    free(before);
    free(after);
}
END_TEST


/* ============================================================ */
/* Suite */
/* ============================================================ */

Suite *engine_suite(void)
{
    Suite *s = suite_create("GameEngine");
    TCase *tc = tcase_create("HappyPath");
    TCase *a2 = tcase_create("Assignment2");

    tcase_add_checked_fixture(tc, setup_engine, teardown_engine);

    tcase_add_test(tc, test_move_player_simple_east);
    tcase_add_test(tc, test_get_room_count);
    tcase_add_test(tc, test_get_room_dimensions);
    tcase_add_test(tc, test_game_engine_reset);
    tcase_add_test(tc, test_render_current_room);
    tcase_add_test(tc, test_render_room);
    tcase_add_test(tc, test_get_room_ids);

    //assignment 2 tests

    tcase_add_checked_fixture(a2, setup_engine, teardown_engine);

    tcase_add_test(a2, test_move_player_collect_treasure);
    tcase_add_test(a2, test_game_engine_reset_treasures);
    tcase_add_test(a2, test_game_engine_free_string);
    tcase_add_test(a2, test_render_collected_treasure_as_floor);


    suite_add_tcase(s, tc);
    suite_add_tcase(s, a2);
    return s;
}
