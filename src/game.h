#ifndef GAME_H
#define GAME_H

#include "chess.h"

typedef void (*init_t)(void *memory, unsigned long size);
typedef void (*move_t)(chess_state_t *board);

extern void init_player1_fn(void *memory, unsigned long size);
extern void init_player2_fn(void *memory, unsigned long size);

extern void move_player1_fn(chess_state_t *board);
extern void move_player2_fn(chess_state_t *board);

void init(void *memory, unsigned long size);
void tick(float deltaTime);

#endif // GAME_H
