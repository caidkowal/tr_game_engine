#ifndef GAME_ENGINE_EXTRA_H
#define GAME_ENGINE_EXTRA_H

#include "game_engine.h"

Status game_engine_get_total_treasure_count(const GameEngine *eng, int *count_out);

Status game_engine_get_adjacency_matrix(const GameEngine *eng, int **matrix_out, int *size_out);

Status game_engine_peek_tile(const GameEngine *eng, Direction dir, int *tile_type_out);

Status game_engine_use_portal(GameEngine *eng);

Status game_engine_set_player_position(GameEngine *eng, int x, int y);

#endif