#include "world_loader.h"
#include "datagen.h"
#include "graph.h"
#include "room.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//helper function to clean up if something fails
static void cleanup_on_error(Graph *graph, bool datagen_started) {
    if (graph) {
        graph_destroy(graph);
    }
    if (datagen_started) {
        stop_datagen();
    }
}

//compare two rooms by id (used by graph)
static int room_compare(const void *a, const void *b) {
    const Room *ra = (const Room *)a;
    const Room *rb = (const Room *)b;

    if (!ra || !rb) return 0;

    if (ra->id < rb->id) return -1;
    if (ra->id > rb->id) return 1;
    return 0;
}

//helper to load portals from datagen room
static Portal* load_portals(const DG_Room *dg_room, int *error) {
    *error = 0;
    
    //no portals to load
    if (dg_room->portal_count == 0) {
        return NULL;
    }
    
    //allocate portal array
    Portal *new_portals = malloc(dg_room->portal_count * sizeof(Portal));
    if (!new_portals) {
        *error = 1;
        return NULL;
    }
    
    //copy each portal from datagen
    for (int i = 0; i < dg_room->portal_count; i++) {
        new_portals[i].id = dg_room->portals[i].id;
        new_portals[i].x = dg_room->portals[i].x;
        new_portals[i].y = dg_room->portals[i].y;
        new_portals[i].target_room_id = dg_room->portals[i].neighbor_id;
        new_portals[i].name = NULL;
    }
    
    return new_portals;
}

//helper to load treasures from datagen room
static Treasure* load_treasures(const DG_Room *dg_room, int *error) {
    *error = 0;
    
    //no treasures to load
    if (dg_room->treasure_count == 0) {
        return NULL;
    }
    
    //allocate treasure array
    Treasure *new_treasures = malloc(dg_room->treasure_count * sizeof(Treasure));
    if (!new_treasures) {
        *error = 1;
        return NULL;
    }
    
    //copy each treasure from datagen
    for (int i = 0; i < dg_room->treasure_count; i++) {
        new_treasures[i].id = dg_room->treasures[i].global_id;
        new_treasures[i].x = dg_room->treasures[i].x;
        new_treasures[i].y = dg_room->treasures[i].y;
        new_treasures[i].initial_x = dg_room->treasures[i].x;
        new_treasures[i].initial_y = dg_room->treasures[i].y;
        new_treasures[i].starting_room_id = dg_room->id;
        new_treasures[i].collected = false;
        
        //deep copy treasure name
        new_treasures[i].name = strdup(dg_room->treasures[i].name);
        if (!new_treasures[i].name) {
            //cleanup on failure
            for (int j = 0; j < i; j++) {
                free(new_treasures[j].name);
            }
            free(new_treasures);
            *error = 1;
            return NULL;
        }
    }
    
    return new_treasures;
}

//helper to load pushables from datagen room
static Pushable* load_pushables(const DG_Room *dg_room, int *error) {
    *error = 0;
    
    //no pushables to load
    if (dg_room->pushable_count == 0) {
        return NULL;
    }
    
    //allocate pushable array
    Pushable *new_pushables = malloc(dg_room->pushable_count * sizeof(Pushable));
    if (!new_pushables) {
        *error = 1;
        return NULL;
    }
    
    //copy each pushable from datagen
    for (int i = 0; i < dg_room->pushable_count; i++) {
        new_pushables[i].id = dg_room->pushables[i].id;
        new_pushables[i].x = dg_room->pushables[i].x;
        new_pushables[i].y = dg_room->pushables[i].y;
        new_pushables[i].initial_x = dg_room->pushables[i].x;
        new_pushables[i].initial_y = dg_room->pushables[i].y;
        
        //deep copy pushable name
        new_pushables[i].name = strdup(dg_room->pushables[i].name);
        if (!new_pushables[i].name) {
            //cleanup on failure
            for (int j = 0; j < i; j++) {
                free(new_pushables[j].name);
            }
            free(new_pushables);
            *error = 1;
            return NULL;
        }
    }
    
    return new_pushables;
}

//helper to create and populate a single room from datagen data
static Room* create_room_from_datagen(const DG_Room *dg_room, int *error) {
    *error = 0;
    
    //create our own room struct
    Room *new_room = room_create(dg_room->id, NULL, dg_room->width, dg_room->height);
    if (!new_room) {
        *error = 1;
        return NULL;
    }

    //deep copy floor grid
    int grid_size = dg_room->width * dg_room->height;
    bool *new_grid = malloc(grid_size * sizeof(bool));
    if (!new_grid) {
        room_destroy(new_room);
        *error = 1;
        return NULL;
    }
    memcpy(new_grid, dg_room->floor_grid, grid_size * sizeof(bool));
    room_set_floor_grid(new_room, new_grid);

    //load portals using helper function
    int load_error = 0;
    Portal *new_portals = load_portals(dg_room, &load_error);
    if (load_error) {
        room_destroy(new_room);
        *error = 1;
        return NULL;
    }
    room_set_portals(new_room, new_portals, dg_room->portal_count);

    //load treasures using helper function
    Treasure *new_treasures = load_treasures(dg_room, &load_error);
    if (load_error) {
        room_destroy(new_room);
        *error = 1;
        return NULL;
    }
    room_set_treasures(new_room, new_treasures, dg_room->treasure_count);

    //load pushables using helper function
    Pushable *new_pushables = load_pushables(dg_room, &load_error);
    if (load_error) {
        room_destroy(new_room);
        *error = 1;
        return NULL;
    }
    new_room->pushables = new_pushables;
    new_room->pushable_count = dg_room->pushable_count;

    return new_room;
}

//helper to connect graph edges using portal neighbor ids
static Status connect_rooms(Graph *graph) {
    const void * const *payloads = NULL;
    int count = 0;
    
    //get all rooms from graph
    GraphStatus gs = graph_get_all_payloads(graph, &payloads, &count);
    if (gs != GRAPH_STATUS_OK) {
        return INTERNAL_ERROR;
    }
    
    //loop through all rooms
    for (int i = 0; i < count; i++) {
        Room *from = (Room *)payloads[i];
        
        //loop through each portal in the room
        for (int j = 0; j < from->portal_count; j++) {
            int target_id = from->portals[j].target_room_id;
            
            //skip if portal doesn't connect to a room
            if (target_id < 0) {
                continue;
            }
            
            //create lookup key to find target room
            Room key;
            memset(&key, 0, sizeof(Room));
            key.id = target_id;
            
            //find the target room
            Room *to = (Room *)graph_get_payload(graph, &key);
            if (!to) {
                return GE_NO_SUCH_ROOM;
            }
            
            //connect the rooms in the graph
            gs = graph_connect(graph, from, to);
            if (gs == GRAPH_STATUS_NO_MEMORY) {
                return NO_MEMORY;
            }
            if (gs != GRAPH_STATUS_OK && gs != GRAPH_STATUS_DUPLICATE_EDGE) {
                return INTERNAL_ERROR;
            }
        }
    }
    
    return OK;
}

Status loader_load_world(const char *config_file, Graph **graph_out, Room **first_room_out, int *num_rooms_out, Charset *charset_out)
{
    //check that inputs are valid
    if (!config_file || !graph_out || !first_room_out || !num_rooms_out || !charset_out) {
        return INVALID_ARGUMENT;
    }

    //initialize output parameters
    *graph_out = NULL;
    *first_room_out = NULL;
    *num_rooms_out = 0;

    //start datagen using config file
    int dg_status = start_datagen(config_file);
    if (dg_status == DG_ERR_CONFIG) return WL_ERR_CONFIG;
    if (dg_status == DG_ERR_OOM) return NO_MEMORY;
    if (dg_status != DG_OK) return WL_ERR_DATAGEN;

    bool datagen_started = true;

    //copy charset from datagen
    const DG_Charset *dg_charset = dg_get_charset();
    if (!dg_charset) {
        cleanup_on_error(NULL, datagen_started);
        return WL_ERR_DATAGEN;
    }

    //copy all charset characters
    charset_out->wall = dg_charset->wall;
    charset_out->floor = dg_charset->floor;
    charset_out->player = dg_charset->player;
    charset_out->treasure = dg_charset->treasure;
    charset_out->portal = dg_charset->portal;
    charset_out->pushable = dg_charset->pushable;

    //create graph that will own all rooms
    Graph *graph = NULL;
    GraphStatus gs = graph_create(room_compare, (GraphDestroyFn)room_destroy, &graph);

    if (gs == GRAPH_STATUS_NO_MEMORY) {
        cleanup_on_error(NULL, datagen_started);
        return NO_MEMORY;
    }
    if (gs != GRAPH_STATUS_OK) {
        cleanup_on_error(NULL, datagen_started);
        return INTERNAL_ERROR;
    }

    Room *first_room = NULL;

    //loop through every room returned by datagen
    while (has_more_rooms()) {
        DG_Room dg_room = get_next_room();
        
        //create and populate room from datagen data
        int error = 0;
        Room *new_room = create_room_from_datagen(&dg_room, &error);
        if (error) {
            cleanup_on_error(graph, datagen_started);
            return NO_MEMORY;
        }

        //insert room into graph
        gs = graph_insert(graph, new_room);
        if (gs == GRAPH_STATUS_NO_MEMORY) {
            room_destroy(new_room);
            cleanup_on_error(graph, datagen_started);
            return NO_MEMORY;
        }
        if (gs != GRAPH_STATUS_OK) {
            room_destroy(new_room);
            cleanup_on_error(graph, datagen_started);
            return INTERNAL_ERROR;
        }

        //store first room inserted
        if (!first_room) {
            first_room = new_room;
        }
    }

    //connect graph edges using portal neighbor ids
    Status connect_status = connect_rooms(graph);
    if (connect_status != OK) {
        cleanup_on_error(graph, datagen_started);
        return connect_status;
    }

    //stop datagen since we copied everything
    stop_datagen();

    //set output parameters
    *graph_out = graph;
    *first_room_out = first_room;
    *num_rooms_out = graph_size(graph);

    return OK;
}