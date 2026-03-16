#include "room.h"

#include <stdlib.h>
#include <string.h>

Room *room_create(int id, const char *name, int width, int height){
    Room *new_room = malloc(sizeof(Room));
    if (!new_room) {
        return NULL;
    }

    if (name == NULL) {
        new_room->name = strdup("");   // empty string
    } else {
        new_room->name = strdup(name);
    }

    if (!new_room->name) {
        free(new_room);
        return NULL;
    }

    new_room->id = id;

    //set width and height to be atleast 1
    if(width < 1){
        width = 1;
    }
    if(height < 1){
        height = 1;
    }
    new_room->width = width;
    new_room->height = height;

    new_room->floor_grid = NULL;
    new_room->portals = NULL;
    new_room->portal_count = 0;
    new_room->treasures = NULL;
    new_room->treasure_count = 0;

    new_room->pushables = NULL;
    new_room->pushable_count = 0;

    new_room->neighbors = NULL;
    new_room->neighbor_count = 0;

    new_room->switches = NULL;
    new_room->switch_count = 0;

    return new_room;
}


int room_get_width(const Room *r){

    if (!r){
        return 0;
    }

    return r->width;
}

int room_get_height(const Room *r){

    if (!r){
        return 0;
    }

    return r->height;
}

Status room_set_floor_grid(Room *r, bool *floor_grid){

    if(!r){
        return INVALID_ARGUMENT;
    }

    if(r->floor_grid){
        free(r->floor_grid);
    }

    r->floor_grid = floor_grid;


    return OK;
}

Status room_set_portals(Room *r, Portal *portals, int portal_count){

    if(!r || (portal_count>0 && !portals )){
        return INVALID_ARGUMENT;
    }

    if (r->portals) {
        for (int i = 0; i < r->portal_count; i++) {
            free(r->portals[i].name);
        }
        free(r->portals);
    }

    r->portals = portals;
    r->portal_count = portal_count;

    return OK;

}

Status room_set_treasures(Room *r, Treasure *treasures, int treasure_count){

    if(!r || (treasure_count>0 && !treasures )){
        return INVALID_ARGUMENT;
    }

    if (r->treasures) {
        for (int i = 0; i < r->treasure_count; i++) {
            free(r->treasures[i].name);
        }
        free(r->treasures);
    }

    r->treasures = treasures;
    r->treasure_count = treasure_count;

    return OK;

}

Status room_place_treasure(Room *r, const Treasure *treasure){

    if (!r || !treasure) {
        return INVALID_ARGUMENT;
    }
    
    Treasure *tmp = realloc(r->treasures, sizeof(Treasure) * (r->treasure_count + 1));
    if (!tmp) {
        return NO_MEMORY;
    }

    r->treasures = tmp;

    //copy the new treasure into the last slot
    r->treasures[r->treasure_count] = *treasure;

    r->treasure_count += 1;

    return OK;

}

int room_get_treasure_at(const Room *r, int x, int y){

    if (!r) {
        return -1;
    }

    //go through each treasure in the room, find which x,y matches
    //only return treasure if it has NOT been collected
    for (int i=0; i < r->treasure_count; i++){
        if (!r->treasures[i].collected &&
            r->treasures[i].x == x && r->treasures[i].y == y){
            return r->treasures[i].id;
        }
    }

    return -1;
}

int room_get_portal_destination(const Room *r, int x, int y){
    
    if (!r) {
        return -1;
    }

    //go through each portal in the room, find which x,y matches
    for (int i=0; i < r->portal_count; i++){
        if (r->portals[i].x == x && r->portals[i].y == y){
            return r->portals[i].target_room_id;
        }
    }

    return -1;
}

bool room_is_walkable(const Room *r, int x, int y){
    if (!r) {
        return false;
    }
    
    //check if out of bounds
    if (x < 0 || y < 0 || x >= r->width || y >= r->height) {
        return false;
    }
    
    //perimeter walls
    if (x == 0 || y == 0 || x == r->width - 1 || y == r->height - 1) {
        return false;
    }

    //check if a pushable is blocking this tile
    for (int i = 0; i < r->pushable_count; i++) {
        if (r->pushables[i].x == x && r->pushables[i].y == y) {
            return false;
        }
    }
    
    //interior is all floors
    if (r->floor_grid == NULL) {
        return true;
    }

    //interior walls/floors
    return r->floor_grid[y * r->width + x];
}

RoomTileType room_classify_tile(const Room *r, int x, int y, int *out_id){

    if (!r) {
        return ROOM_TILE_INVALID;
    }
    
    //check if out of bounds
    if (x < 0 || y < 0 || x >= r->width || y >= r->height) {
        return ROOM_TILE_INVALID;
    }

    //check for pushable first (before walkability, since pushables block)
    for (int i = 0; i < r->pushable_count; i++) {
        if (r->pushables[i].x == x && r->pushables[i].y == y) {
            if (out_id != NULL) {
                *out_id = i;
            }
            return ROOM_TILE_PUSHABLE;
        }
    }

    //check for uncollected treasure
    for (int i = 0; i < r->treasure_count; i++) {
        if (!r->treasures[i].collected &&
            r->treasures[i].x == x && r->treasures[i].y == y) {
            if (out_id != NULL) {
                *out_id = r->treasures[i].id;
            }
            return ROOM_TILE_TREASURE;
        }
    }

    //check for portal
    int portal_dest = room_get_portal_destination(r, x, y);
    if (portal_dest != -1) {
        if (out_id != NULL) {
            *out_id = portal_dest;
        }
        return ROOM_TILE_PORTAL;
    }

    //if true, it is walkable/floor
    if (room_is_walkable(r, x, y)) {
        return ROOM_TILE_FLOOR;
    }
    //else its a wall
    return ROOM_TILE_WALL;


}

Status room_render(const Room *r, const Charset *charset, char *buffer, 
                   int buffer_width, int buffer_height){

    if(!r || !charset || !buffer){
        return INVALID_ARGUMENT;
    }

    if(buffer_width != r->width || buffer_height != r->height){
        return INVALID_ARGUMENT;
    }

    //floors and walls
    for(int row = 0; row < buffer_height; row++){
        for(int col = 0; col < buffer_width; col++){
            int index = row * buffer_width + col;
            
            //check if we are at a perimeter
            if (col == 0 || row == 0 || col == buffer_width - 1 || row == buffer_height - 1) {
                buffer[index] = charset->wall;
            }
            //if floorgrid is null, assume the interior is all walkable
            else if (r->floor_grid == NULL) {
                buffer[index] = charset->floor;
            }
            //use the value from the floor grid, true=floor, false=wall
            else {
                if (r->floor_grid[index]) {
                    buffer[index] = charset->floor;
                } else {
                    buffer[index] = charset->wall;
                }
            }

        }
    }
    //overlay treasures (only uncollected)
    for (int i = 0; i < r->treasure_count; i++) {
        if (!r->treasures[i].collected) {
            int index = r->treasures[i].y * buffer_width + r->treasures[i].x;
            buffer[index] = charset->treasure;
        }
    }

    //overlay portals
    for (int i = 0; i < r->portal_count; i++) {
        int index = r->portals[i].y * buffer_width + r->portals[i].x;
        buffer[index] = charset->portal;
    }

    //overlay pushables
    for (int i = 0; i < r->pushable_count; i++) {
        int index = r->pushables[i].y * buffer_width + r->pushables[i].x;
        buffer[index] = charset->pushable;
    }

    return OK;

}

Status room_get_start_position(const Room *r, int *x_out, int *y_out){
    if (!r || !x_out || !y_out) {
        return INVALID_ARGUMENT;
    }

    //try to start at portal if there is one
    if (r->portal_count > 0) {
        *x_out = r->portals[0].x;
        *y_out = r->portals[0].y;
        return OK;
    }

    //if not, try for any interior tile / look for first tile that is true (floor)
    for (int row = 1; row < r->height - 1; row++) {
        for (int col = 1; col < r->width - 1; col++) {
            if (room_is_walkable(r, col, row)) {
                *x_out = col;
                *y_out = row;
                return OK;
            }
        }
    }

    return ROOM_NOT_FOUND;
}

void room_destroy(Room *r){
    if (!r){
        return;
    }
    free(r->name);

    free(r->floor_grid);

    if (r->portals) {
        for (int i = 0; i < r->portal_count; i++) {
            free(r->portals[i].name); 
        }
        free(r->portals);
    }

    if (r->treasures) {
        for (int i = 0; i < r->treasure_count; i++) {
            free(r->treasures[i].name);
        }
        free(r->treasures);
    }

    if (r->pushables) {
        for (int i = 0; i < r->pushable_count; i++) {
            free(r->pushables[i].name);
        }
        free(r->pushables);
    }

    free(r->neighbors);
    free(r->switches);

    free(r);
}

int room_get_id(const Room *r){

    if(!r){
        return -1;
    }
    return r->id;
}

Status room_pick_up_treasure(Room *r, int treasure_id, Treasure **treasure_out){

    if(!r || !treasure_out || treasure_id<0){
        return INVALID_ARGUMENT;
    }
    
    for (int i=0; i<r->treasure_count; i++){
        if (r->treasures[i].id == treasure_id){

            //check if already collected
            if(r->treasures[i].collected){
                return INVALID_ARGUMENT;
            }

            r->treasures[i].collected = true;

            *treasure_out = &(r->treasures[i]);

            return OK;
        }
    }

    return ROOM_NOT_FOUND;
}

void destroy_treasure(Treasure *t){
    
    //check for null
    if(!t){
        return;
    }
    
    //free the name
    free(t->name);
    
    //free the treasure itself
    free(t);
}


bool room_has_pushable_at(const Room *r, int x, int y, int *pushable_idx_out){
    
    //check for null room
    if(!r){
        return false;
    }
    
    //loop through pushables to find match
    for(int i = 0; i < r->pushable_count; i++){
        if(r->pushables[i].x == x && r->pushables[i].y == y){
            
            //set output index if provided
            if(pushable_idx_out != NULL){
                *pushable_idx_out = i;
            }
            
            return true;
        }
    }
    
    //no pushable at position
    return false;
}

Status room_try_push(Room *r, int pushable_idx, Direction dir){
    
    //check for null room
    if(!r){
        return INVALID_ARGUMENT;
    }
    
    //check pushable_idx bounds
    if(pushable_idx < 0 || pushable_idx >= r->pushable_count){
        return INVALID_ARGUMENT;
    }
    
    //check direction validity
    if(dir < DIR_NORTH || dir > DIR_WEST){
        return INVALID_ARGUMENT;
    }
    
    //get current pushable position
    int current_x = r->pushables[pushable_idx].x;
    int current_y = r->pushables[pushable_idx].y;
    
    //calculate new position based on direction
    int new_x = current_x;
    int new_y = current_y;
    
    switch(dir){
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
    
    //check if new position is walkable
    if (!room_is_walkable(r, new_x, new_y)) {
        return ROOM_IMPASSABLE;
    }

    //pushables cannot be pushed into portal tiles
    if (room_get_portal_destination(r, new_x, new_y) >= 0) {
        return ROOM_IMPASSABLE;
    }
    
    //update pushable position
    r->pushables[pushable_idx].x = new_x;
    r->pushables[pushable_idx].y = new_y;
    
    return OK;
}