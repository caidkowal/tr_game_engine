#include "game_engine.h"
#include "world_loader.h"
#include "datagen.h"
#include "graph.h"
#include "room.h"
#include "player.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

Status game_engine_create(const char *config_file_path, GameEngine **engine_out){

    if(!config_file_path || !engine_out){
        return INVALID_ARGUMENT;
    }

    *engine_out = NULL;

    Graph* new_graph = NULL;
    Room* first_room = NULL;
    int num_rooms = 0;
    Charset charset;

    Status ls = loader_load_world(config_file_path, &new_graph, &first_room, &num_rooms, &charset);
    if (ls == NO_MEMORY) {
        return NO_MEMORY;
    }

    if (ls != OK) {
        return WL_ERR_DATAGEN;
    }

    GameEngine* game_engine = malloc(sizeof(GameEngine));
    if (!game_engine) {
        graph_destroy(new_graph);
        return NO_MEMORY;
    }

    game_engine->initial_room_id = first_room->id;
    game_engine->room_count = num_rooms;
    game_engine->charset = charset;
    game_engine->graph = new_graph;

    int starting_x = 0;
    int starting_y = 0;

    Status rs = room_get_start_position(first_room, &starting_x, &starting_y);
    if (rs != OK) {
        graph_destroy(new_graph);
        free(game_engine);
        return WL_ERR_DATAGEN;
    }

    Player* player_out = NULL;

    Status ps = player_create(game_engine->initial_room_id, starting_x, starting_y, &player_out);
    if (ps == NO_MEMORY) {
        graph_destroy(new_graph);
        free(game_engine);
        return NO_MEMORY;
    }

    if (ps != OK) {
        graph_destroy(new_graph);
        free(game_engine);
        return WL_ERR_DATAGEN;
    }

    game_engine->player = player_out;

    game_engine->initial_player_x = starting_x;
    game_engine->initial_player_y = starting_y;


    *engine_out = game_engine;

    return OK;

}

void game_engine_destroy(GameEngine *eng){

    if (!eng) {
        return;
    }

    if (eng->player) {
        player_destroy(eng->player);
    }

    if (eng->graph) {
        graph_destroy(eng->graph);
    }

    free(eng);
}

const Player *game_engine_get_player(const GameEngine *eng){

    if (!eng) {
        return NULL;
    }

    return eng->player;
}

Status game_engine_move_player(GameEngine *eng, Direction dir){

    if (!eng) {
        return INVALID_ARGUMENT;
    }

    if (!eng->player || !eng->graph) {
        return INTERNAL_ERROR;
    }

    //get current room id
    int current_room_id = player_get_room(eng->player);

    //build lookup key to find room in graph
    Room key;
    memset(&key, 0, sizeof(Room));
    key.id = current_room_id;

    Room *current_room = (Room *)graph_get_payload(eng->graph, &key);

    if (!current_room) {
        return GE_NO_SUCH_ROOM;
    }

    //get current position
    int x = 0;
    int y = 0;

    Status ps = player_get_position(eng->player, &x, &y);
    if (ps != OK) {
        return INTERNAL_ERROR;
    }

    //compute next position
    int new_x = x;
    int new_y = y;

    switch (dir) {
        case DIR_NORTH:
            new_y--;
            break;

        case DIR_SOUTH:
            new_y++;
            break;

        case DIR_WEST:
            new_x--;
            break;

        case DIR_EAST:
            new_x++;
            break;

        default:
            return INVALID_ARGUMENT;
    }

    //check if there is a pushable at the target tile
    int pushable_idx = -1;
    if (room_has_pushable_at(current_room, new_x, new_y, &pushable_idx)) {
        //try to push it
        Status push_status = room_try_push(current_room, pushable_idx, dir);
        if (push_status != OK) {
            return ROOM_IMPASSABLE;
        }
        //pushable was moved, player can step into that tile
        player_set_position(eng->player, new_x, new_y);
        return OK;
    }

    //check if there is an uncollected treasure at the target tile
    int treasure_id = room_get_treasure_at(current_room, new_x, new_y);
    if (treasure_id >= 0) {
        //find treasure pointer directly in the room's array
        for (int i = 0; i < current_room->treasure_count; i++) {
            if (current_room->treasures[i].id == treasure_id) {
                player_try_collect(eng->player, &current_room->treasures[i]);
                break;
            }
        }
        return OK;
    }

    //check if stepping on portal
    int destination = room_get_portal_destination(current_room, new_x, new_y);

    if (destination >= 0) {

        //make sure destination room exists
        key.id = destination;

        Room *target = (Room *)graph_get_payload(eng->graph, &key);

        if (!target){
            return GE_NO_SUCH_ROOM;
        }

        //move player to new room
        player_move_to_room(eng->player, destination);

        //get starting position of new room
        int start_x = 0;
        int start_y = 0;

        Status room_status = room_get_start_position(target, &start_x, &start_y);

        if (room_status != OK) {
            return INTERNAL_ERROR;
        }

        player_set_position(eng->player, start_x, start_y);

        return OK;
    }

    //check walkable (walls, floor_grid)
    if (!room_is_walkable(current_room, new_x, new_y)) {
        return ROOM_IMPASSABLE;
    }

    //normal floor movement
    player_set_position(eng->player, new_x, new_y);

    return OK;
}

Status game_engine_get_room_count(const GameEngine *eng, int *count_out){

    if(!eng){
        return INVALID_ARGUMENT;
    }

    if(!count_out){
        return NULL_POINTER;
    }

    *count_out = eng->room_count;

    return OK;
}

Status game_engine_get_room_dimensions(const GameEngine *eng, int *width_out, int *height_out){

    if(!eng){
        return INVALID_ARGUMENT;
    }

    if(!width_out||!height_out){
        return NULL_POINTER;
    }

    if (!eng->player || !eng->graph) {
        return INTERNAL_ERROR;
    }

    //get current room id
    int current_room_id = player_get_room(eng->player);

    //build lookup key to find room in graph
    Room key;
    memset(&key, 0, sizeof(Room));
    key.id = current_room_id;

    Room *current_room = (Room *)graph_get_payload(eng->graph, &key);

    if (!current_room) {
        return GE_NO_SUCH_ROOM;
    }

    *width_out = room_get_width(current_room);
    *height_out = room_get_height(current_room);

    return OK;

}

Status game_engine_reset(GameEngine *eng){

    if(!eng){
        return INVALID_ARGUMENT;
    }

    if(!eng->player){
        return INVALID_ARGUMENT;
    }

    Status ps = player_reset_to_start(eng->player, eng->initial_room_id, eng->initial_player_x, eng->initial_player_y);

    if(ps != OK){
        return INTERNAL_ERROR;
    }

    const void * const *payloads = NULL;
    int count = 0;

   GraphStatus gs = graph_get_all_payloads(eng->graph, &payloads, &count);

    if (gs != GRAPH_STATUS_OK) {
        return INTERNAL_ERROR;
    }

    //reset treasures and pushables in each room
    for (int i = 0; i < count; i++) {
        Room *room = (Room *)payloads[i];

        //reset all treasures in this room
        for (int j = 0; j < room->treasure_count; j++) {
            room->treasures[j].collected = false;
            room->treasures[j].x = room->treasures[j].initial_x;
            room->treasures[j].y = room->treasures[j].initial_y;
        }

        //reset all pushables in this room
        for (int j = 0; j < room->pushable_count; j++) {
            room->pushables[j].x = room->pushables[j].initial_x;
            room->pushables[j].y = room->pushables[j].initial_y;
        }
    }
    return OK;

}

Status game_engine_render_current_room(const GameEngine *eng,
                                       char **str_out){
    if (!eng || !str_out) {
        return INVALID_ARGUMENT;
    }

    if (!eng->player || !eng->graph) {
        return INTERNAL_ERROR;
    }

    *str_out = NULL;

    //find current room
    int room_id = player_get_room(eng->player);

    //create key to find room
    Room key;
    memset(&key, 0, sizeof(Room));
    key.id = room_id;

    //find room that matches the key
    Room *room = (Room *)graph_get_payload(eng->graph, &key);

    if (!room) {
        return INTERNAL_ERROR;
    }

    int width = room_get_width(room);
    int height = room_get_height(room);

    //allocate raw buffer for rendering (no newlines yet)
    char *grid = malloc( (unsigned long)width * height);
    if (!grid) {
        return NO_MEMORY;
    }

    Status rs = room_render(room, &eng->charset, grid, width, height);

    if (rs != OK) {
        free(grid);
        return INTERNAL_ERROR;
    }

    //overlay player
    int px = 0;
    int py = 0;

    Status player_status = player_get_position(eng->player, &px, &py);

    if (player_status != OK) {
        free(grid);
        return INTERNAL_ERROR;
    }

    //put an @ where the player is 
    grid[py * width + px] = eng->charset.player;

    //allocate final string with +1 for newlines
    int final_size = height * (width + 1) + 1;


    //create final printable string
    char *result = malloc(final_size);
    if (!result) {
        free(grid);
        return NO_MEMORY;
    }

    int index = 0;
    
    //populate final string, convert 2D grid into 1D array
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {

            //copy char from grid into result
            result[index++] = grid[row * width + col];
        }
        result[index++] = '\n';
    }

    //add null terminator at the end 
    result[index] = '\0';

    free(grid);

    *str_out = result;

    return OK;
}

Status game_engine_render_room(const GameEngine *eng, int room_id, char **str_out){
    if (!eng || !str_out) {
        return INVALID_ARGUMENT;
    }

    if (!eng->player || !eng->graph) {
        return INTERNAL_ERROR;
    }

    *str_out = NULL;

    //create key to find room
    Room key;
    memset(&key, 0, sizeof(Room));
    key.id = room_id;

    //find room that matches the key
    Room *room = (Room *)graph_get_payload(eng->graph, &key);

    if (!room) {
        return GE_NO_SUCH_ROOM;
    }

    int width = room_get_width(room);
    int height = room_get_height(room);

    //allocate raw buffer for rendering (no newlines yet)
    char *grid = malloc( (unsigned long)width * height);
    if (!grid) {
        return NO_MEMORY;
    }

    Status rs = room_render(room, &eng->charset, grid, width, height);

    if (rs != OK) {
        free(grid);
        return INTERNAL_ERROR;
    }

    //allocate final string with +1 for newlines
    int final_size = height * (width + 1) + 1;


    //create final printable string
    char *result = malloc(final_size);
    if (!result) {
        free(grid);
        return NO_MEMORY;
    }

    int index = 0;
    
    //populate final string, convert 2D grid into 1D array
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {

            //copy char from grid into result
            result[index++] = grid[row * width + col];
        }
        result[index++] = '\n';
    }

    //add null terminator at the end
    result[index] = '\0';

    free(grid);

    *str_out = result;

    return OK;
}

Status game_engine_get_room_ids(const GameEngine *eng, int **ids_out, int *count_out){
    
    if(!eng){
        return INVALID_ARGUMENT;
    }

    if(!ids_out || !count_out){
        return NULL_POINTER;
    }

    if (!eng->graph){
        return INTERNAL_ERROR;
    } 

    *ids_out = NULL;
    *count_out = 0;

    const void * const *payloads = NULL;
    int count = 0;

    GraphStatus gs = graph_get_all_payloads(eng->graph, &payloads, &count);

    if (gs!=0){
        return INTERNAL_ERROR;
    }
    

    int *ids = malloc(sizeof(int) * count);
    if (!ids){
        return NO_MEMORY;
    }

    for (int i = 0; i < count; i++) {
        Room *room = (Room *)payloads[i];
        ids[i] = room->id;
    }

    *ids_out = ids;
    *count_out = count;

    return OK;


}

void game_engine_free_string(void *ptr){
    free(ptr);
}

//added for a3
//all treasures victory screen extended feature
Status game_engine_get_total_treasure_count(const GameEngine *eng, int *count_out) {
    if (!eng) {
        return INVALID_ARGUMENT;
    }

    if (!count_out) {
        return NULL_POINTER;
    }

    if (!eng->graph) {
        return INTERNAL_ERROR;
    }

    const void * const *payloads = NULL;
    int room_count = 0;

    GraphStatus gs = graph_get_all_payloads(eng->graph, &payloads, &room_count);
    if (gs != GRAPH_STATUS_OK) {
        return INTERNAL_ERROR;
    }

    int total = 0;
    for (int i = 0; i < room_count; i++) {
        const Room *room = (const Room *)payloads[i];
        total += room->treasure_count;
    }

    *count_out = total;
    return OK;
}

Status game_engine_get_adjacency_matrix(const GameEngine *eng, int **matrix_out, int *size_out) {
    if (!eng) {
        return INVALID_ARGUMENT;
    }

    if (!matrix_out || !size_out) {
        return NULL_POINTER;
    }

    if (!eng->graph) {
        return INTERNAL_ERROR;
    }

    const void * const *payloads = NULL;
    int count = 0;

    GraphStatus gs = graph_get_all_payloads(eng->graph, &payloads, &count);
    if (gs != GRAPH_STATUS_OK) {
        return INTERNAL_ERROR;
    }

    int *matrix = calloc((size_t)count * (size_t)count, sizeof(int));
    if (!matrix) {
        return NO_MEMORY;
    }

    for (int i = 0; i < count; i++) {
        const Room *room = (const Room *)payloads[i];

        // --- ONLY use portals (safe and correct) ---
        for (int p = 0; p < room->portal_count; p++) {
            int target = room->portals[p].target_room_id;

            if (target < 0) continue;

            int target_index = -1;

            for (int j = 0; j < count; j++) {
                const Room *candidate = (const Room *)payloads[j];
                if (candidate->id == target) {
                    target_index = j;
                    break;
                }
            }

            if (target_index >= 0) {
                matrix[i * count + target_index] = 1;
            }
        }
    }

    *matrix_out = matrix;
    *size_out = count;
    return OK;
}