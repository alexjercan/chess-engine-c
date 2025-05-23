#ifndef GAME_H
#define GAME_H

#include "chess.h"

typedef void (*init_fn)(void *memory, unsigned long size);
typedef void (*move_fn)(const chess_state_t *state, move_t *choices, int count, int *index);

extern void init_player1_fn(void *memory, unsigned long size);
extern void init_player2_fn(void *memory, unsigned long size);

extern void move_player1_fn(const chess_state_t *state, move_t *choices, int count, int *index);
extern void move_player2_fn(const chess_state_t *state, move_t *choices, int count, int *index);

void init(void *memory, unsigned long size);
void state_init(char *fen);

void tick(float deltaTime);

#endif // GAME_H
