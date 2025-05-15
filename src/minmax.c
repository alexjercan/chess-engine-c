#ifdef __wasm__
#include "wasm.h"
#else
#include <time.h>
#endif

#include "chess.h"
#include "util.h"

extern DS_ALLOCATOR allocator;

#ifndef MINMAX_DEPTH
#define MINMAX_DEPTH 3
#endif

#define MINMAX_INF 1000000

typedef struct move_score {
    int move;
    int score;
} move_score;

#define MK_MOVE_SCORE(m, s) (move_score){ .move = (m), .score = (s)}

static int eval(const chess_state_t *state, char current);
static move_score minmax(const chess_state_t *state, move_t *choices, int count, char maxxing, int depth);

static int eval(const chess_state_t *state, char current) {
    int material1 = chess_count_material(state, current);
    int material2 = chess_count_material(state, chess_flip_player(current));

    return material1 - material2;
}

static move_score minmax(const chess_state_t *state, move_t *choices, int count, char maxxing, int depth) {
    char result = chess_checkmate(state);
    if (result != CHESS_NONE) {
        return MK_MOVE_SCORE(-1, (maxxing == result) ? -MINMAX_INF : MINMAX_INF);
    }

    if (chess_draw(state)) {
        return MK_MOVE_SCORE(-1, 0);
    }

    if (depth == 0) {
        return MK_MOVE_SCORE(-1, eval(state, maxxing));
    }

    move_score best = {.score = 0, .move = -1};
    if (maxxing == state->current_player) best.score = -MINMAX_INF;
    else best.score = MINMAX_INF;
    best.move = (count == 0) ? -1 : rand() % count;

    for (int i = 0; i < count; i++) {
        chess_state_t clone = {0};
        DS_MEMCPY(&clone, state, sizeof(chess_state_t));

        move_t move = choices[i];
        chess_apply_move(&clone, move.start, move.end, move.move);
        if (move.promotion != CHESS_NONE) {
            chess_square_set(&clone.board, move.end, move.promotion);
        }
        clone.current_player = chess_flip_player(clone.current_player);

        ds_dynamic_array moves = {0}; /* move_t */
        ds_dynamic_array_init_allocator(&moves, sizeof(move_t), allocator);
        chess_generate_moves(&clone, &moves);

        move_score value = minmax(&clone, moves.items, moves.count, maxxing, depth - 1);

        ds_dynamic_array_free(&moves);

        if (maxxing == state->current_player) {
            if (value.score > best.score) {
                best.score = value.score;
                best.move = i;
            }
        } else {
            if (value.score < best.score) {
                best.score = value.score;
                best.move = i;
            }
        }
    }

    return best;
}

void chess_init(void *memory, unsigned long size) {
    util_init(memory, size);

#ifdef __wasm__
#else
    srand(time(NULL));
#endif
}

void chess_move(const chess_state_t *state, move_t *choices, int count, int *index) {
    *index = minmax(state, choices, count, state->current_player, MINMAX_DEPTH).move;
}

