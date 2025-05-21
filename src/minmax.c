#ifdef __wasm__
#include "wasm.h"
#else
#include <time.h>
#endif

#include "chess.h"
#include "util.h"

#ifndef MINMAX_DEPTH
#define MINMAX_DEPTH 4
#endif

static int eval(const chess_state_t *state, char current) {
    int material1 = chess_count_material_weighted(state, current);
    int material2 = chess_count_material_weighted(state, chess_flip_player(current));

    return material1 - material2;
}

static int sort(const void *a, const void *b) {
    move_t *move_a = (move_t *)a;
    move_t *move_b = (move_t *)b;

    if ((move_a->move & CHESS_CAPTURE) != 0) return -1;
    if ((move_b->move & CHESS_CAPTURE) != 0) return 1;

    return 0;
}

void chess_init(void *memory, unsigned long size) {
    util_init(memory, size);

#ifdef __wasm__
#else
    srand(time(NULL));
#endif
}

void chess_move(const chess_state_t *state, move_t *choices, int count, int *index) {
    minmax_info info = {0};

    clock_t start = clock();

    move_score s = minmax(state, choices, count, state->current_player, MINMAX_DEPTH,
                          -MINMAX_INF, MINMAX_INF, eval, sort, &info);

    clock_t end = clock();

    DS_LOG_DEBUG("Minmax took %f seconds", (double)(end - start) / CLOCKS_PER_SEC);
    DS_LOG_DEBUG("Evaluated %d positions", info.positions);
    DS_LOG_DEBUG("Evaluation: %d", s.score);

    *index = s.move;
}

