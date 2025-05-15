#ifdef __wasm__
#include "wasm.h"
#else
#include <time.h>
#endif

#include "chess.h"
#include "util.h"

void chess_init(void *memory, unsigned long size) {
    util_init(memory, size);

#ifdef __wasm__
#else
    srand(time(NULL));
#endif
}

void chess_move(const chess_state_t *state, move_t *choices, int count, int *index) {
    UNUSED(state);
    UNUSED(choices);

    *index = rand() % count;
}
