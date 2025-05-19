#ifndef UTIL_H
#define UTIL_H

#ifdef __wasm__
#include "wasm.h"
#else
#include "raylib.h"
#endif

#include "chess.h"

#ifndef ASSETS_FOLDER
#define ASSETS_FOLDER "dist/assets/"
#endif

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800

#define PROMOTION_GUI_HEIGHT 150
#define PROMOTION_GUI_WIDTH 4 * PROMOTION_GUI_HEIGHT

#define MAX_CAPACITY 100

#define MINMAX_INF 1000000

typedef struct move_score {
    int move;
    int score;
} move_score;

typedef struct minmax_info {
    int positions;
} minmax_info;

#define MK_MOVE_SCORE(m, s) (move_score){ .move = (m), .score = (s)}

typedef int(eval_fn)(const chess_state_t *, char);

Texture2D LoadTextureCachedPiece(char piece);
Sound LoadSoundCachedMove(char move);

void px_to_square(Vector2 *px, square_t *square);
void square_to_px(square_t *square, Vector2 *px);
int px_to_option(Vector2 *px);

unsigned long long_hash(const void *key);
int long_compare(const void *k1, const void *k2);

void util_init(void *memory, unsigned long size);
void *util_malloc(unsigned long size);
void util_free(void *ptr);

move_score minmax(const chess_state_t *state, move_t *choices, int count,
                  char maxxing, int depth, int alpha, int beta, eval_fn *eval,
                  minmax_info *info);

#endif // UTIL_H
