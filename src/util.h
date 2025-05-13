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
Texture2D LoadTextureCachedMap(ds_hashmap *textures, const char *fileName);
Texture2D LoadTextureCached(const char *fileName);

void px_to_square(Vector2 *px, square_t *square);
void square_to_px(square_t *square, Vector2 *px);
int px_to_option(Vector2 *px);

unsigned long string_hash(const void *key);
int string_compare(const void *k1, const void *k2);

void util_init(void *memory, unsigned long size);
void *util_malloc(unsigned long size);
void util_free(void *ptr);

#endif // UTIL_H
