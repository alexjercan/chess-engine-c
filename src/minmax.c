#ifdef __wasm__
#include "wasm.h"
#else
#include <time.h>
#endif

#include "chess.h"
#include "util.h"

#ifndef MINMAX_DEPTH
#define MINMAX_DEPTH 3
#endif

static int eval(const chess_state_t *state, char current) {
    int material1 = chess_count_material(state, current);
    int material2 = chess_count_material(state, chess_flip_player(current));

    return material1 - material2;
}

void chess_init(void *memory, unsigned long size) {
    util_init(memory, size);

#ifdef __wasm__
#else
    srand(time(NULL));
#endif
}

void chess_move(const chess_state_t *state, move_t *choices, int count, int *index) {
    *index = minmax(state, choices, count, state->current_player, MINMAX_DEPTH,
                    -MINMAX_INF, MINMAX_INF, eval).move;
}

