#ifndef UTIL_H
#define UTIL_H

#ifdef __wasm__
#include "wasm.h"
#else
#include "raylib.h"
#endif

#include "chess.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800

#define PROMOTION_GUI_HEIGHT 150
#define PROMOTION_GUI_WIDTH 4 * PROMOTION_GUI_HEIGHT

#define MAX_CAPACITY 100

const char *chess_piece_texture_path(char piece);
Texture2D LoadTextureCached(ds_hashmap *textures, const char *fileName);

void px_to_square(Vector2 *px, square_t *square);
void square_to_px(square_t *square, Vector2 *px);

unsigned long string_hash(const void *key);
int string_compare(const void *k1, const void *k2);

#endif // UTIL_H
