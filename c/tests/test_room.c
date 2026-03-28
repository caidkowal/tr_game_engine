#include <check.h>
#include <stdlib.h>
#include "room.h"


/* ============================================================
 * Setup and Teardown fixtures
 * ============================================================ */

static Room *r = NULL;

static void setup_room(void)
{
    r = room_create(1, "NewRoom", 2, 3);
}

static void teardown_room(void)
{
    room_destroy(r);
    r = NULL;
}


/* ============================================================
 * room_create test
 * ============================================================ */
START_TEST(test_room_create)
{
    ck_assert_ptr_nonnull(r);
    ck_assert_int_eq(r->id, 1);
    ck_assert_int_eq(r->width, 2);
    ck_assert_int_eq(r->height, 3);
}
END_TEST


/* ============================================================
 * room name copy test
 * ============================================================ */
START_TEST(test_room_name_copy)
{
    ck_assert_ptr_nonnull(r);
    ck_assert_ptr_nonnull(r->name);

    ck_assert_str_eq(r->name, "NewRoom");
    ck_assert_ptr_ne(r->name, "NewRoom");
}
END_TEST


/* ============================================================
 * Getter tests
 * ============================================================ */
START_TEST(test_room_get_width)
{
    ck_assert_int_eq(room_get_width(r), 2);
}
END_TEST


START_TEST(test_room_get_height)
{
    ck_assert_int_eq(room_get_height(r), 3);
}
END_TEST

/* ============================================================
 * room_set_floor_grid overwrite test
 * ============================================================ */
START_TEST(test_room_set_floor_grid_overwrite)
{
    // 2 & 3 is the size of the room
    bool *grid1 = malloc(2 * 3 * sizeof(bool));
    bool *grid2 = malloc(2 * 3 * sizeof(bool));

    ck_assert_ptr_nonnull(grid1);
    ck_assert_ptr_nonnull(grid2);

    ck_assert_int_eq(room_set_floor_grid(r, grid1), OK);
    ck_assert_ptr_eq(r->floor_grid, grid1);

    ck_assert_int_eq(room_set_floor_grid(r, grid2), OK);
    ck_assert_ptr_eq(r->floor_grid, grid2);
}
END_TEST

/* ============================================================
 * room_set_portals overwrite test
 * ============================================================ */
START_TEST(test_room_set_portals_overwrite)
{
    Portal *portals1 = malloc(sizeof(Portal) * 2);
    Portal *portals2 = malloc(sizeof(Portal) * 1);

    ck_assert_ptr_nonnull(portals1);
    ck_assert_ptr_nonnull(portals2);

    portals1[0].name = strdup("A");
    portals1[1].name = strdup("B");

    portals2[0].name = strdup("C");

    ck_assert_int_eq(room_set_portals(r, portals1, 2), OK);
    ck_assert_ptr_eq(r->portals, portals1);
    ck_assert_int_eq(r->portal_count, 2);

    ck_assert_int_eq(room_set_portals(r, portals2, 1), OK);
    ck_assert_ptr_eq(r->portals, portals2);
    ck_assert_int_eq(r->portal_count, 1);
}
END_TEST

/* ============================================================
 * room_set_treasures success test
 * ============================================================ */
START_TEST(test_room_set_treasures_success)
{
    Treasure *treasures = malloc(sizeof(Treasure) * 2);
    ck_assert_ptr_nonnull(treasures);

    // treasure 1
    treasures[0].id = 1;
    treasures[0].name = strdup("ruby");
    treasures[0].starting_room_id = 1;

    treasures[0].initial_x = 0;
    treasures[0].initial_y = 0;
    treasures[0].x = 0;
    treasures[0].y = 0;

    treasures[0].collected = false;

    // treasure 2 
    treasures[1].id = 2;
    treasures[1].name = strdup("sword");
    treasures[1].starting_room_id = 1;

    treasures[1].initial_x = 2;
    treasures[1].initial_y = 1;
    treasures[1].x = 2;
    treasures[1].y = 1;

    treasures[1].collected = false;

    Status s = room_set_treasures(r, treasures, 2);

    // verification 
    ck_assert_int_eq(s, OK);
    ck_assert_ptr_eq(r->treasures, treasures);
    ck_assert_int_eq(r->treasure_count, 2);

    ck_assert_int_eq(r->treasures[0].id, 1);
    ck_assert_str_eq(r->treasures[0].name, "ruby");

    ck_assert_int_eq(r->treasures[1].id, 2);
    ck_assert_str_eq(r->treasures[1].name, "sword");
}
END_TEST

START_TEST(test_room_place_treasure_simple)
{
    Treasure t;
    t.id = 1;
    t.name = strdup("ruby");
    t.starting_room_id = 1;

    t.initial_x = 0;
    t.x = 0;

    t.initial_y = 0;
    t.y = 0;
    t.collected = false;

    Status s = room_place_treasure(r, &t);

    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(r->treasure_count, 1);
    ck_assert_str_eq(r->treasures[0].name, "ruby");
    ck_assert_int_eq(r->treasures[0].id, 1);
}
END_TEST

START_TEST(test_room_get_treasure_at_simple)
{
    Treasure t;
    t.id = 7;
    t.name = strdup("ruby");
    t.x = 2;
    t.y = 3;
    t.collected = false;

    //place treasure into room
    ck_assert_int_eq(room_place_treasure(r, &t), OK);

    ck_assert_int_eq(room_get_treasure_at(r, 2, 3), 7);

    //no treasure at 0,0
    ck_assert_int_eq(room_get_treasure_at(r, 0, 0), -1);
}
END_TEST

START_TEST(test_room_get_portal_dest_simple)
{

    //create porals and portal array
    Portal p1;
    p1.id = 1;
    p1.x = 2;
    p1.y = 3;
    p1.name = strdup("Portal1");
    p1.target_room_id = 10;

    Portal p2;
    p2.id = 4;
    p2.x = 5;
    p2.y = 6;
    p2.name = strdup("Portal2");
    p2.target_room_id = 99;

    Portal *portalsArr = malloc(sizeof(Portal) * 2);
    portalsArr[0] = p1;
    portalsArr[1] = p2;

    //set the portals
    ck_assert_int_eq(room_set_portals(r, portalsArr, 2), OK);

    //get portal 1 dest room id , which is 10
    ck_assert_int_eq(room_get_portal_destination(r,2,3), 10);

    //get portal 2 dest room id , which is 99
    ck_assert_int_eq(room_get_portal_destination(r,5,6), 99);

    // no portal 
    ck_assert_int_eq(room_get_portal_destination(r,9,9), -1);


}
END_TEST

START_TEST(test_room_is_walkable)
{
    // Create a room specifically for this test with correct dimensions
    Room *test_room = room_create(99, "TestRoom", 4, 3);
    ck_assert_ptr_nonnull(test_room);

    //room dimensions
    int width = 4;
    int height = 3;

    //allocate floor grid (flattened 1d array)
    bool *grid = malloc(width * height * sizeof(bool));
    ck_assert_ptr_nonnull(grid);

    //layout: t = walkable, f = wall
    //row-major order (y * width + x)
    //0: t f t t
    //1: t t f t
    //2: t t t f
    grid[0 * width + 0] = true;  grid[0 * width + 1] = false;
    grid[0 * width + 2] = true;  grid[0 * width + 3] = true;

    grid[1 * width + 0] = true;  grid[1 * width + 1] = true;
    grid[1 * width + 2] = false; grid[1 * width + 3] = true;

    grid[2 * width + 0] = true;  grid[2 * width + 1] = true;
    grid[2 * width + 2] = true;  grid[2 * width + 3] = false;

    //set the floor grid in the room
    ck_assert_int_eq(room_set_floor_grid(test_room, grid), OK);

    //interior walkable
    ck_assert(room_is_walkable(test_room, 1, 1) == true);

    //interior wall
    ck_assert(room_is_walkable(test_room, 2, 1) == false);

    //perimeter walls
    ck_assert(room_is_walkable(test_room, 0, 0) == false);
    ck_assert(room_is_walkable(test_room, 3, 2) == false);

    //out of bounds
    ck_assert(room_is_walkable(test_room, -1, 0) == false);
    ck_assert(room_is_walkable(test_room, 4, 1) == false);
    
    //cleann up
    room_destroy(test_room);
}
END_TEST

START_TEST(test_room_classify_tile_simple)
{
    //create a 5x5 room
    Room *test_room = room_create(1, "ClassifyTest", 5, 5);
    ck_assert_ptr_nonnull(test_room);

    //create floor grid (5x5 = 25 tiles)
    bool *grid = malloc(5 * 5 * sizeof(bool));
    ck_assert_ptr_nonnull(grid);
    
    //set all interior to walkable, perimeter handled by room_is_walkable
    for (int i = 0; i < 25; i++) {
        grid[i] = true;
    }
    //add one interior wall at (2, 2)
    grid[2 * 5 + 2] = false;
    
    room_set_floor_grid(test_room, grid);

    //add a treasure at (2, 3)
    Treasure t;
    t.id = 100;
    t.name = strdup("ruby");
    t.x = 2;
    t.y = 3;
    t.collected = false;
    room_place_treasure(test_room, &t);

    //add a portal at (3, 1)
    Portal *portals = malloc(sizeof(Portal));
    portals[0].id = 1;
    portals[0].name = strdup("portal1");
    portals[0].x = 3;
    portals[0].y = 1;
    portals[0].target_room_id = 99;
    room_set_portals(test_room, portals, 1);

    int out_id;

    //out of bounds
    ck_assert_int_eq(room_classify_tile(test_room, -1, 0, NULL), ROOM_TILE_INVALID);
    ck_assert_int_eq(room_classify_tile(test_room, 5, 2, NULL), ROOM_TILE_INVALID);

    //perimeter wall
    ck_assert_int_eq(room_classify_tile(test_room, 0, 0, NULL), ROOM_TILE_WALL);
    ck_assert_int_eq(room_classify_tile(test_room, 4, 2, NULL), ROOM_TILE_WALL);

    //interior wall
    ck_assert_int_eq(room_classify_tile(test_room, 2, 2, NULL), ROOM_TILE_WALL);

    //treasure (and check out_id)
    ck_assert_int_eq(room_classify_tile(test_room, 2, 3, &out_id), ROOM_TILE_TREASURE);
    ck_assert_int_eq(out_id, 100);  // should be treasure ID

    //portal (and check out_id)
    ck_assert_int_eq(room_classify_tile(test_room, 3, 1, &out_id), ROOM_TILE_PORTAL);
    ck_assert_int_eq(out_id, 99);  // should be destination room ID

    //empty floor
    ck_assert_int_eq(room_classify_tile(test_room, 1, 1, NULL), ROOM_TILE_FLOOR);
    ck_assert_int_eq(room_classify_tile(test_room, 3, 3, NULL), ROOM_TILE_FLOOR);

    //NULL room
    ck_assert_int_eq(room_classify_tile(NULL, 1, 1, NULL), ROOM_TILE_INVALID);
}
END_TEST

START_TEST(test_room_render_simple)
{
    //create a small 5x4 room
    Room *test_room = room_create(1, "RenderTest", 5, 4);
    ck_assert_ptr_nonnull(test_room);

    // Create floor grid (5x4 = 20 tiles)
    bool *grid = malloc(5 * 4 * sizeof(bool));
    ck_assert_ptr_nonnull(grid);
    
    //set all to true (floor)
    for (int i = 0; i < 20; i++) {
        grid[i] = true;
    }
    //add one interior wall at (2, 2)
    grid[2 * 5 + 2] = false;
    
    room_set_floor_grid(test_room, grid);

    //add a treasure at (1, 1)
    Treasure t;
    t.id = 1;
    t.name = strdup("coin");
    t.x = 1;
    t.y = 1;
    t.collected = false;
    room_place_treasure(test_room, &t);

    //addd a portal at (3, 2)
    Portal *portals = malloc(sizeof(Portal));
    portals[0].id = 1;
    portals[0].name = strdup("Door");
    portals[0].x = 3;
    portals[0].y = 2;
    portals[0].target_room_id = 5;
    portals[0].gated = false;
    portals[0].required_switch_id = -1;
    room_set_portals(test_room, portals, 1);

    //create charset
    Charset charset;
    charset.wall = '#';
    charset.floor = '.';
    charset.treasure = '$';
    charset.portal = 'X';
    charset.player = '@';
    charset.switch_off = '=';
    charset.switch_on = '+';
    charset.pushable = 'O';

    //create buffer
    char buffer[20]; // 5 * 4 = 20
    
    //render the room
    Status s = room_render(test_room, &charset, buffer, 5, 4);
    ck_assert_int_eq(s, OK);

    // ROOM
    // ROW 0: #####
    // ROW 1: #$..#
    // ROW 2: #..X#
    // ROW 3: ##### 
    
    //check perimeter walls (row 0)
    ck_assert_int_eq(buffer[0 * 5 + 0], '#');
    ck_assert_int_eq(buffer[0 * 5 + 1], '#');
    ck_assert_int_eq(buffer[0 * 5 + 4], '#');
    
    //check treasure at (1, 1)
    ck_assert_int_eq(buffer[1 * 5 + 1], '$');
    
    //check portal at (3, 2) - open portal shows 'X'
    ck_assert_int_eq(buffer[2 * 5 + 3], 'X');
    
    //check interior wall at (2, 2)
    ck_assert_int_eq(buffer[2 * 5 + 2], '#');
    
    //check regular floor at (2, 1)
    ck_assert_int_eq(buffer[1 * 5 + 2], '.');
    
    //check perimeter walls (bottom row)
    ck_assert_int_eq(buffer[3 * 5 + 0], '#');
    ck_assert_int_eq(buffer[3 * 5 + 4], '#');

}
END_TEST

START_TEST(test_room_get_start_position_simple)
{
    int x_out, y_out;
    Status s;

    //room with portal - should return portal location
    Room *room1 = room_create(1, "Test", 5, 5);
    
    Portal *portals = malloc(sizeof(Portal));
    portals[0].id = 1;
    portals[0].name = strdup("Exit");
    portals[0].x = 3;
    portals[0].y = 2;
    portals[0].target_room_id = 10;
    room_set_portals(room1, portals, 1);
    
    s = room_get_start_position(room1, &x_out, &y_out);
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(x_out, 3);
    ck_assert_int_eq(y_out, 2);
    
    room_destroy(room1);

    //room without portal - should return first walkable tile (1,1)
    Room *room2 = room_create(2, "Test2", 5, 5);
    
    s = room_get_start_position(room2, &x_out, &y_out);
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(x_out, 1);
    ck_assert_int_eq(y_out, 1);
    
    room_destroy(room2);

    //nuLL room
    s = room_get_start_position(NULL, &x_out, &y_out);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

/* ============================================================
 * A2 Room Tests
 * ============================================================ */

START_TEST(test_room_get_id_basic)
{
    //room should return its id
    int id = room_get_id(r);
    ck_assert_int_eq(id, 1);
}
END_TEST

START_TEST(test_room_get_id_null)
{
    //null room should return -1
    int id = room_get_id(NULL);
    ck_assert_int_eq(id, -1);
}
END_TEST

START_TEST(test_room_pick_up_treasure_basic)
{
    //create and add treasure to room
    Treasure t;
    t.id = 50;
    t.name = strdup("Diamond");
    t.x = 2;
    t.y = 2;
    t.collected = false;
    
    room_place_treasure(r, &t);
    
    //pick up the treasure
    Treasure *picked_up = NULL;
    Status s = room_pick_up_treasure(r, 50, &picked_up);
    
    //should succeed
    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(picked_up);
    ck_assert_ptr_eq(picked_up, &(r->treasures[0]));
    ck_assert(picked_up->collected == true);
}
END_TEST

START_TEST(test_room_pick_up_treasure_null_room)
{
    Treasure *picked_up = NULL;
    
    //null room should return invalid_argument
    Status s = room_pick_up_treasure(NULL, 50, &picked_up);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_room_pick_up_treasure_null_output)
{
    //null treasure_out should return invalid_argument
    Status s = room_pick_up_treasure(r, 50, NULL);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_room_pick_up_treasure_not_found)
{
    Treasure *picked_up = NULL;
    
    //treasure id that doesn't exist should return room_not_found
    Status s = room_pick_up_treasure(r, 999, &picked_up);
    ck_assert_int_eq(s, ROOM_NOT_FOUND);
}
END_TEST

START_TEST(test_room_pick_up_treasure_already_collected)
{
    //create and add treasure to room
    Treasure t;
    t.id = 60;
    t.name = strdup("Ruby");
    t.x = 1;
    t.y = 1;
    t.collected = false;
    
    room_place_treasure(r, &t);
    
    //pick up once
    Treasure *picked_up1 = NULL;
    Status s1 = room_pick_up_treasure(r, 60, &picked_up1);
    ck_assert_int_eq(s1, OK);
    
    //try to pick up again - should fail
    Treasure *picked_up2 = NULL;
    Status s2 = room_pick_up_treasure(r, 60, &picked_up2);
    ck_assert_int_eq(s2, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_destroy_treasure_basic)
{
    //create heap-allocated treasure
    Treasure *t = malloc(sizeof(Treasure));
    t->id = 100;
    t->name = strdup("Emerald");
    
    //destroy should not crash
    destroy_treasure(t);
}
END_TEST

START_TEST(test_destroy_treasure_null)
{
    //null should not crash
    destroy_treasure(NULL);
}
END_TEST

START_TEST(test_room_has_pushable_at_true)
{
    //create room with pushable
    Room *test_room = room_create(5, "PushTest", 5, 5);
    
    //add a pushable
    Pushable *pushables = malloc(sizeof(Pushable));
    pushables[0].id = 1;
    pushables[0].name = strdup("Boulder");
    pushables[0].x = 3;
    pushables[0].y = 2;
    
    test_room->pushables = pushables;
    test_room->pushable_count = 1;
    
    //check if pushable exists at position
    int idx = -1;
    bool has_it = room_has_pushable_at(test_room, 3, 2, &idx);
    
    ck_assert(has_it == true);
    ck_assert_int_eq(idx, 0);
    
    room_destroy(test_room);
}
END_TEST

START_TEST(test_room_has_pushable_at_false)
{
    //create room with pushable
    Room *test_room = room_create(5, "PushTest", 5, 5);
    
    //add a pushable
    Pushable *pushables = malloc(sizeof(Pushable));
    pushables[0].id = 1;
    pushables[0].name = strdup("Boulder");
    pushables[0].x = 3;
    pushables[0].y = 2;
    
    test_room->pushables = pushables;
    test_room->pushable_count = 1;
    
    //check different position - should return false
    bool has_it = room_has_pushable_at(test_room, 1, 1, NULL);
    ck_assert(has_it == false);
    
    room_destroy(test_room);
}
END_TEST

START_TEST(test_room_has_pushable_at_null)
{
    //null room should return false
    bool has_it = room_has_pushable_at(NULL, 3, 2, NULL);
    ck_assert(has_it == false);
}
END_TEST

START_TEST(test_room_try_push_basic)
{
    //create 5x5 room with floor
    Room *test_room = room_create(10, "PushTest", 5, 5);
    
    bool *grid = malloc(5 * 5 * sizeof(bool));
    for(int i = 0; i < 25; i++){
        grid[i] = true;
    }
    room_set_floor_grid(test_room, grid);
    
    //add a pushable at (2, 2)
    Pushable *pushables = malloc(sizeof(Pushable));
    pushables[0].id = 1;
    pushables[0].name = strdup("Boulder");
    pushables[0].x = 2;
    pushables[0].y = 2;
    
    test_room->pushables = pushables;
    test_room->pushable_count = 1;
    
    //push east
    Status s = room_try_push(test_room, 0, DIR_EAST);
    
    //should succeed
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(test_room->pushables[0].x, 3);
    ck_assert_int_eq(test_room->pushables[0].y, 2);
    
    room_destroy(test_room);
}
END_TEST

START_TEST(test_room_try_push_blocked)
{
    //create 5x5 room
    Room *test_room = room_create(10, "PushTest", 5, 5);
    
    bool *grid = malloc(5 * 5 * sizeof(bool));
    for(int i = 0; i < 25; i++){
        grid[i] = true;
    }
    //add wall at (3, 2)
    grid[2 * 5 + 3] = false;
    room_set_floor_grid(test_room, grid);
    
    //add pushable at (2, 2)
    Pushable *pushables = malloc(sizeof(Pushable));
    pushables[0].id = 1;
    pushables[0].name = strdup("Boulder");
    pushables[0].x = 2;
    pushables[0].y = 2;
    
    test_room->pushables = pushables;
    test_room->pushable_count = 1;
    
    //try to push east into wall - should fail
    Status s = room_try_push(test_room, 0, DIR_EAST);
    
    ck_assert_int_eq(s, ROOM_IMPASSABLE);
    
    //position should not change
    ck_assert_int_eq(test_room->pushables[0].x, 2);
    ck_assert_int_eq(test_room->pushables[0].y, 2);
    
    room_destroy(test_room);
}
END_TEST

START_TEST(test_room_try_push_null_room)
{
    //null room should return invalid_argument
    Status s = room_try_push(NULL, 0, DIR_NORTH);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_room_try_push_invalid_index)
{
    //create room with one pushable
    Room *test_room = room_create(10, "PushTest", 5, 5);
    
    Pushable *pushables = malloc(sizeof(Pushable));
    pushables[0].id = 1;
    pushables[0].name = strdup("Boulder");
    
    test_room->pushables = pushables;
    test_room->pushable_count = 1;
    
    //try with invalid index - should fail
    Status s = room_try_push(test_room, 5, DIR_NORTH);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
    
    room_destroy(test_room);
}
END_TEST


/* render_switches: switch_off char appears when no pushable on switch */
START_TEST(test_ckowal_room_render_switch_off)
{
    Room *test_room = room_create(1, "SwitchTest", 5, 5);
    ck_assert_ptr_nonnull(test_room);

    bool *grid = malloc(5 * 5 * sizeof(bool));
    for (int i = 0; i < 25; i++) { grid[i] = true; }
    room_set_floor_grid(test_room, grid);

    //add a switch at interior tile (2, 2)
    Switch *switches = malloc(sizeof(Switch));
    switches[0].id = 0;
    switches[0].x = 2;
    switches[0].y = 2;
    switches[0].portal_id = 0;
    test_room->switches = switches;
    test_room->switch_count = 1;

    Charset cs;
    cs.wall = '#'; cs.floor = '.'; cs.treasure = '$';
    cs.portal = 'X'; cs.player = '@';
    cs.switch_off = '='; cs.switch_on = '+'; cs.pushable = 'O';

    char buffer[25];
    Status s = room_render(test_room, &cs, buffer, 5, 5);
    ck_assert_int_eq(s, OK);

    //switch tile should show switch_off since no pushable is on it
    ck_assert_int_eq(buffer[2 * 5 + 2], '=');

    room_destroy(test_room);
}
END_TEST

/* render_switches: switch_on char appears when pushable is on switch */
START_TEST(test_ckowal_room_render_switch_pressed)
{
    Room *test_room = room_create(1, "SwitchOnTest", 5, 5);
    ck_assert_ptr_nonnull(test_room);

    bool *grid = malloc(5 * 5 * sizeof(bool));
    for (int i = 0; i < 25; i++) { grid[i] = true; }
    room_set_floor_grid(test_room, grid);

    //switch at (2, 2)
    Switch *switches = malloc(sizeof(Switch));
    switches[0].id = 0;
    switches[0].x = 2;
    switches[0].y = 2;
    switches[0].portal_id = 0;
    test_room->switches = switches;
    test_room->switch_count = 1;

    //pushable on top of the switch tile (2, 2)
    Pushable *pushables = malloc(sizeof(Pushable));
    pushables[0].id = 0;
    pushables[0].name = strdup("Rock");
    pushables[0].x = 2;
    pushables[0].y = 2;
    test_room->pushables = pushables;
    test_room->pushable_count = 1;

    Charset cs;
    cs.wall = '#'; cs.floor = '.'; cs.treasure = '$';
    cs.portal = 'X'; cs.player = '@';
    cs.switch_off = '='; cs.switch_on = '+'; cs.pushable = 'O';

    char buffer[25];
    Status s = room_render(test_room, &cs, buffer, 5, 5);
    ck_assert_int_eq(s, OK);

    //switch tile should show switch_on (pushable consumed onto switch)
    ck_assert_int_eq(buffer[2 * 5 + 2], '+');

    room_destroy(test_room);
}
END_TEST

/* render_portals: locked portal shows '!' sentinel */
START_TEST(test_ckowal_room_render_locked_portal_sentinel)
{
    Room *test_room = room_create(1, "LockTest", 5, 5);
    ck_assert_ptr_nonnull(test_room);

    bool *grid = malloc(5 * 5 * sizeof(bool));
    for (int i = 0; i < 25; i++) { grid[i] = true; }
    room_set_floor_grid(test_room, grid);

    //switch at (1, 1) - not pressed (no pushable on it)
    Switch *switches = malloc(sizeof(Switch));
    switches[0].id = 0;
    switches[0].x = 1;
    switches[0].y = 1;
    switches[0].portal_id = 0;
    test_room->switches = switches;
    test_room->switch_count = 1;

    //gated portal at (3, 2) controlled by switch 0
    Portal *portals = malloc(sizeof(Portal));
    portals[0].id = 0;
    portals[0].name = strdup("LockedDoor");
    portals[0].x = 3;
    portals[0].y = 2;
    portals[0].target_room_id = 5;
    portals[0].gated = true;
    portals[0].required_switch_id = 0;
    room_set_portals(test_room, portals, 1);

    Charset cs;
    cs.wall = '#'; cs.floor = '.'; cs.treasure = '$';
    cs.portal = 'X'; cs.player = '@';
    cs.switch_off = '='; cs.switch_on = '+'; cs.pushable = 'O';

    char buffer[25];
    Status s = room_render(test_room, &cs, buffer, 5, 5);
    ck_assert_int_eq(s, OK);

    //locked portal should render as '!' sentinel, not 'X'
    ck_assert_int_eq(buffer[2 * 5 + 3], '!');

    room_destroy(test_room);
}
END_TEST

/* room_classify_tile: pushable tile is classified as ROOM_TILE_PUSHABLE */
START_TEST(test_ckowal_room_classify_tile_pushable)
{
    Room *test_room = room_create(1, "ClassifyPush", 5, 5);
    ck_assert_ptr_nonnull(test_room);

    bool *grid = malloc(5 * 5 * sizeof(bool));
    for (int i = 0; i < 25; i++) { grid[i] = true; }
    room_set_floor_grid(test_room, grid);

    //add a pushable at interior tile (2, 3)
    Pushable *pushables = malloc(sizeof(Pushable));
    pushables[0].id = 0;
    pushables[0].name = strdup("Crate");
    pushables[0].x = 2;
    pushables[0].y = 3;
    test_room->pushables = pushables;
    test_room->pushable_count = 1;

    int out_id = -1;
    RoomTileType tile = room_classify_tile(test_room, 2, 3, &out_id);

    //tile should be classified as pushable
    ck_assert_int_eq(tile, ROOM_TILE_PUSHABLE);
    //out_id should be the pushable index (0)
    ck_assert_int_eq(out_id, 0);

    room_destroy(test_room);
}
END_TEST


/* room_try_push north moves pushable up by one row */
START_TEST(test_ckowal_room_try_push_north)
{
    Room *test_room = room_create(10, "PushNorth", 5, 5);

    bool *grid = malloc(5 * 5 * sizeof(bool));
    for (int i = 0; i < 25; i++) { grid[i] = true; }
    room_set_floor_grid(test_room, grid);

    Pushable *pushables = malloc(sizeof(Pushable));
    pushables[0].id = 0;
    pushables[0].name = strdup("Rock");
    pushables[0].x = 2;
    pushables[0].y = 3;
    test_room->pushables = pushables;
    test_room->pushable_count = 1;

    Status s = room_try_push(test_room, 0, DIR_NORTH);
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(test_room->pushables[0].x, 2);
    ck_assert_int_eq(test_room->pushables[0].y, 2);

    room_destroy(test_room);
}
END_TEST

/* room_try_push south moves pushable down by one row */
START_TEST(test_ckowal_room_try_push_south)
{
    Room *test_room = room_create(10, "PushSouth", 5, 5);

    bool *grid = malloc(5 * 5 * sizeof(bool));
    for (int i = 0; i < 25; i++) { grid[i] = true; }
    room_set_floor_grid(test_room, grid);

    Pushable *pushables = malloc(sizeof(Pushable));
    pushables[0].id = 0;
    pushables[0].name = strdup("Rock");
    pushables[0].x = 2;
    pushables[0].y = 2;
    test_room->pushables = pushables;
    test_room->pushable_count = 1;

    Status s = room_try_push(test_room, 0, DIR_SOUTH);
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(test_room->pushables[0].x, 2);
    ck_assert_int_eq(test_room->pushables[0].y, 3);

    room_destroy(test_room);
}
END_TEST

/* room_try_push west moves pushable left by one column */
START_TEST(test_ckowal_room_try_push_west)
{
    Room *test_room = room_create(10, "PushWest", 5, 5);

    bool *grid = malloc(5 * 5 * sizeof(bool));
    for (int i = 0; i < 25; i++) { grid[i] = true; }
    room_set_floor_grid(test_room, grid);

    Pushable *pushables = malloc(sizeof(Pushable));
    pushables[0].id = 0;
    pushables[0].name = strdup("Rock");
    pushables[0].x = 3;
    pushables[0].y = 2;
    test_room->pushables = pushables;
    test_room->pushable_count = 1;

    Status s = room_try_push(test_room, 0, DIR_WEST);
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(test_room->pushables[0].x, 2);
    ck_assert_int_eq(test_room->pushables[0].y, 2);

    room_destroy(test_room);
}
END_TEST

/* room_try_push into a portal tile returns ROOM_IMPASSABLE */
START_TEST(test_ckowal_room_try_push_into_portal_blocked)
{
    Room *test_room = room_create(10, "PushPortal", 5, 5);

    bool *grid = malloc(5 * 5 * sizeof(bool));
    for (int i = 0; i < 25; i++) { grid[i] = true; }
    room_set_floor_grid(test_room, grid);

    //portal at (3, 2) - pushing east into it should be blocked
    Portal *portals = malloc(sizeof(Portal));
    portals[0].id = 0;
    portals[0].name = strdup("Door");
    portals[0].x = 3;
    portals[0].y = 2;
    portals[0].target_room_id = 5;
    portals[0].gated = false;
    portals[0].required_switch_id = -1;
    room_set_portals(test_room, portals, 1);

    Pushable *pushables = malloc(sizeof(Pushable));
    pushables[0].id = 0;
    pushables[0].name = strdup("Rock");
    pushables[0].x = 2;
    pushables[0].y = 2;
    test_room->pushables = pushables;
    test_room->pushable_count = 1;

    //pushing east would land on the portal tile - should return ROOM_IMPASSABLE
    Status s = room_try_push(test_room, 0, DIR_EAST);
    ck_assert_int_eq(s, ROOM_IMPASSABLE);

    //pushable position must not have changed
    ck_assert_int_eq(test_room->pushables[0].x, 2);
    ck_assert_int_eq(test_room->pushables[0].y, 2);

    room_destroy(test_room);
}
END_TEST

/* collected treasure is not returned by room_get_treasure_at */
START_TEST(test_ckowal_room_get_treasure_at_collected_skipped)
{
    Room *test_room = room_create(1, "CollectTest", 5, 5);
    ck_assert_ptr_nonnull(test_room);

    Treasure t;
    t.id = 77;
    t.name = strdup("Jewel");
    t.x = 2;
    t.y = 2;
    t.collected = true;   //already collected
    room_place_treasure(test_room, &t);

    //should return -1 because treasure is already collected
    ck_assert_int_eq(room_get_treasure_at(test_room, 2, 2), -1);

    room_destroy(test_room);
}
END_TEST

/* collected treasure is classified as floor, not treasure */
START_TEST(test_ckowal_room_classify_collected_treasure_is_floor)
{
    Room *test_room = room_create(1, "CollectedFloor", 5, 5);
    ck_assert_ptr_nonnull(test_room);

    bool *grid = malloc(5 * 5 * sizeof(bool));
    for (int i = 0; i < 25; i++) { grid[i] = true; }
    room_set_floor_grid(test_room, grid);

    //place a collected treasure at (2, 2)
    Treasure t;
    t.id = 88;
    t.name = strdup("OldGold");
    t.x = 2;
    t.y = 2;
    t.collected = true;
    room_place_treasure(test_room, &t);

    //collected treasure should appear as FLOOR, not TREASURE
    RoomTileType tile = room_classify_tile(test_room, 2, 2, NULL);
    ck_assert_int_eq(tile, ROOM_TILE_FLOOR);

    room_destroy(test_room);
}
END_TEST

/* room_set_floor_grid with null room returns INVALID_ARGUMENT */
START_TEST(test_ckowal_room_set_floor_grid_null_room)
{
    bool *grid = malloc(4 * sizeof(bool));
    Status s = room_set_floor_grid(NULL, grid);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
    free(grid);
}
END_TEST


/* ============================================================
 * Suite Creation Function
 * ============================================================ */
Suite *room_suite(void)
{
    Suite *s = suite_create("Room");
    TCase *tc = tcase_create("HappyPath");
    TCase *a2 = tcase_create("Assignment2");
    TCase *a3 = tcase_create("Assignment3");
    TCase *a3b = tcase_create("Assignment3Extra");

    tcase_add_checked_fixture(tc, setup_room, teardown_room);

    tcase_add_test(tc, test_room_create);
    tcase_add_test(tc, test_room_name_copy);
    tcase_add_test(tc, test_room_get_width);
    tcase_add_test(tc, test_room_get_height);
    tcase_add_test(tc, test_room_set_floor_grid_overwrite);

    tcase_add_test(tc, test_room_set_portals_overwrite);
    tcase_add_test(tc, test_room_set_treasures_success);
    tcase_add_test(tc, test_room_place_treasure_simple);
    tcase_add_test(tc, test_room_get_treasure_at_simple);
    tcase_add_test(tc, test_room_get_portal_dest_simple);

    tcase_add_test(tc, test_room_is_walkable);
    tcase_add_test(tc, test_room_classify_tile_simple);
    tcase_add_test(tc, test_room_render_simple);
    tcase_add_test(tc, test_room_get_start_position_simple);

    //assignments 2 tests
    tcase_add_checked_fixture(a2, setup_room, teardown_room);

    tcase_add_test(a2, test_room_get_id_basic);
    tcase_add_test(a2, test_room_get_id_null);
    
    tcase_add_test(a2, test_room_pick_up_treasure_basic);
    tcase_add_test(a2, test_room_pick_up_treasure_null_room);
    tcase_add_test(a2, test_room_pick_up_treasure_null_output);
    tcase_add_test(a2, test_room_pick_up_treasure_not_found);
    tcase_add_test(a2, test_room_pick_up_treasure_already_collected);
    
    tcase_add_test(a2, test_destroy_treasure_basic);
    tcase_add_test(a2, test_destroy_treasure_null);
    
    tcase_add_test(a2, test_room_has_pushable_at_true);
    tcase_add_test(a2, test_room_has_pushable_at_false);
    tcase_add_test(a2, test_room_has_pushable_at_null);
    
    tcase_add_test(a2, test_room_try_push_basic);
    tcase_add_test(a2, test_room_try_push_blocked);
    tcase_add_test(a2, test_room_try_push_null_room);
    tcase_add_test(a2, test_room_try_push_invalid_index);

    //assignment 3 tests
    tcase_add_checked_fixture(a3, setup_room, teardown_room);

    tcase_add_test(a3, test_ckowal_room_render_switch_off);
    tcase_add_test(a3, test_ckowal_room_render_switch_pressed);
    tcase_add_test(a3, test_ckowal_room_render_locked_portal_sentinel);
    tcase_add_test(a3, test_ckowal_room_classify_tile_pushable);

    //assignment 3 extra coverage
    tcase_add_checked_fixture(a3b, setup_room, teardown_room);

    tcase_add_test(a3b, test_ckowal_room_try_push_north);
    tcase_add_test(a3b, test_ckowal_room_try_push_south);
    tcase_add_test(a3b, test_ckowal_room_try_push_west);
    tcase_add_test(a3b, test_ckowal_room_try_push_into_portal_blocked);
    tcase_add_test(a3b, test_ckowal_room_get_treasure_at_collected_skipped);
    tcase_add_test(a3b, test_ckowal_room_classify_collected_treasure_is_floor);
    tcase_add_test(a3b, test_ckowal_room_set_floor_grid_null_room);

    suite_add_tcase(s, tc);
    suite_add_tcase(s, a2);
    suite_add_tcase(s, a3);
    suite_add_tcase(s, a3b);

    return s;
}