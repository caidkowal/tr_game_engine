#include "player.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


Status player_create(int initial_room_id, int initial_x, int initial_y, Player **player_out){


    if (!player_out) {
        return INVALID_ARGUMENT;
    }

    Player *new_player = malloc(sizeof(Player));
    if (!new_player){
        return NO_MEMORY;
    }

    new_player->room_id = initial_room_id;
    new_player->x = initial_x;
    new_player->y = initial_y;
    new_player->collected_treasures = NULL;
    new_player->collected_count = 0;

    *player_out = new_player;

    return OK; 
}


void player_destroy(Player *p){

    if (!p) {
        return;
    }

    free(p->collected_treasures);
    free(p);
}

int player_get_room(const Player *p){

    if (!p){
        return -1;
    }

    return p->room_id;
}

Status player_get_position(const Player *p, int *x_out, int *y_out){

    if (!p || !x_out || !y_out){
        return INVALID_ARGUMENT;
    }

    *x_out = p->x;
    *y_out = p->y;

    return OK;
}

Status player_set_position(Player *p, int x, int y){

    if (!p){
        return INVALID_ARGUMENT;
    }

    p->x = x;
    p->y = y;

    return OK;
}

Status player_move_to_room(Player *p, int new_room_id){

    if (!p){
        return INVALID_ARGUMENT;
    }

    p->room_id = new_room_id;

    return OK;
    
}

Status player_reset_to_start(Player *p,int starting_room_id, int start_x, int start_y){

    if (!p){
        return INVALID_ARGUMENT;
    }

    p->room_id = starting_room_id;
    p->x = start_x;
    p->y = start_y;

    //clears collected treasures
    free(p->collected_treasures);
    p->collected_treasures = NULL;
    p->collected_count = 0;
    return OK;
}

Status player_try_collect(Player *p, Treasure *treasure){

    if(!p || !treasure){
        return NULL_POINTER;
    }

    if(treasure->collected){
        return INVALID_ARGUMENT;
    }

    //check if treasure is there already
    for(int i = 0; i < p->collected_count; i++){
        if(p->collected_treasures[i] == treasure){
            return INVALID_ARGUMENT;
        }
    }

    //p->collected_treasures has been expanded to accommodate the new treasure
    Treasure **tmp = realloc(p->collected_treasures, sizeof(Treasure*) * (p->collected_count+1));
    if (!tmp){
        return NO_MEMORY;
    }
    p->collected_treasures = tmp;

    //treasure ptr has been correctly added to p->collected_treasures array
    p->collected_treasures[p->collected_count] = treasure;  
    p->collected_count++;

    //treasure->collected status has been correctly updated
    treasure->collected = true;
    
    return OK;
}

bool player_has_collected_treasure(const Player *p, int treasure_id){

    if(!p || treasure_id<0){
        return false;
    }

    //go through treasures
    for(int i = 0; i < p->collected_count; i++){
        if(p->collected_treasures[i]->id == treasure_id){
            return true;
        }
    }
    return false;
}

int player_get_collected_count(const Player *p){

    if(!p){
        return 0;
    }

    return p->collected_count;

}

const Treasure * const *
 player_get_collected_treasures(const Player *p, int *count_out){

    if(!p || !count_out){
        return NULL;
    }

    *count_out = p->collected_count;

    //cast to const to indicate read-only
    return (const Treasure * const *)p->collected_treasures;
}