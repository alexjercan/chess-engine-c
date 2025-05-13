#include "util.h"

DS_ALLOCATOR allocator = {0};
static ds_hashmap textures = {0};

#ifndef ASSETS_FOLDER
#define ASSETS_FOLDER "dist/assets/"
#endif

const char *chess_piece_texture_path(char piece) {
    switch (piece) {
        case CHESS_PAWN | CHESS_WHITE:
            return ASSETS_FOLDER "Chess_plt60.png";
        case CHESS_PAWN | CHESS_BLACK:
            return ASSETS_FOLDER "Chess_pdt60.png";
        case CHESS_ROOK | CHESS_WHITE:
            return ASSETS_FOLDER "Chess_rlt60.png";
        case CHESS_ROOK | CHESS_BLACK:
            return ASSETS_FOLDER "Chess_rdt60.png";
        case CHESS_KNIGHT | CHESS_WHITE:
            return ASSETS_FOLDER "Chess_nlt60.png";
        case CHESS_KNIGHT | CHESS_BLACK:
            return ASSETS_FOLDER "Chess_ndt60.png";
        case CHESS_BISHOP | CHESS_WHITE:
            return ASSETS_FOLDER "Chess_blt60.png";
        case CHESS_BISHOP | CHESS_BLACK:
            return ASSETS_FOLDER "Chess_bdt60.png";
        case CHESS_QUEEN | CHESS_WHITE:
            return ASSETS_FOLDER "Chess_qlt60.png";
        case CHESS_QUEEN | CHESS_BLACK:
            return ASSETS_FOLDER "Chess_qdt60.png";
        case CHESS_KING | CHESS_WHITE:
            return ASSETS_FOLDER "Chess_klt60.png";
        case CHESS_KING | CHESS_BLACK:
            return ASSETS_FOLDER "Chess_kdt60.png";
        default:
            DS_PANIC("Chess piece is not supported %d", piece);
            return NULL;
    }
}

Texture2D LoadTextureCachedMap(ds_hashmap *textures, const char *fileName) {
    Texture2D texture = {0};
    ds_hashmap_kv kv = { .key = (void *)fileName, .value = NULL };
    DS_UNREACHABLE(ds_hashmap_get_or_default(textures, &kv, NULL));

    if (kv.value != NULL) {
        DS_MEMCPY(&texture, kv.value, sizeof(Texture2D));
        return texture;
    }

    texture = LoadTexture(fileName);
    kv.value = DS_MALLOC(textures->allocator, sizeof(Texture2D));
    DS_MEMCPY(kv.value, &texture, sizeof(Texture2D));
    ds_hashmap_insert(textures, &kv);

    return texture;
}

Texture2D LoadTextureCached(const char *fileName) {
    if (textures.capacity == 0) {
        if (ds_hashmap_init_allocator(&textures, MAX_CAPACITY, string_hash, string_compare, &allocator) != DS_OK) {
            DS_PANIC("Error initializing hashmap");
        }
    }

    return LoadTextureCachedMap(&textures, fileName);
}

unsigned long string_hash(const void *key) {
    unsigned long hash = 0;
    char *name = (char *)key;
    for (unsigned int i = 0; i < DS_STRLEN(name); i++) {
        hash = 31 * hash + name[i];
    }
    return hash % MAX_CAPACITY;
}

int string_compare(const void *k1, const void *k2) {
    return DS_STRCMP((char *)k1, (char *)k2);
}

void px_to_square(Vector2 *px, square_t *square) {
    int cell_width = SCREEN_WIDTH / CHESS_WIDTH;
    int cell_height = SCREEN_HEIGHT / CHESS_HEIGHT;

    square->file = (int)px->x / cell_width;
    square->rank = (CHESS_HEIGHT - (int)px->y / cell_height - 1);
}

void square_to_px(square_t *square, Vector2 *px) {
    int cell_width = SCREEN_WIDTH / CHESS_WIDTH;
    int cell_height = SCREEN_HEIGHT / CHESS_HEIGHT;

    px->x = square->file * cell_width;
    px->y = (CHESS_WIDTH - square->rank - 1) * cell_height;
}

int px_to_option(Vector2 *px) {
    int col_px = SCREEN_WIDTH / 2 - PROMOTION_GUI_WIDTH / 2;
    int row_px = SCREEN_HEIGHT / 2 - PROMOTION_GUI_HEIGHT / 2;

    if (px->x < col_px || px->x > col_px + PROMOTION_GUI_WIDTH || px->y < row_px || px->y > row_px + PROMOTION_GUI_HEIGHT) {
        return -1;
    } else {
        return (px->x - col_px) / PROMOTION_GUI_HEIGHT;
    }
}

void util_init(void *memory, unsigned long size) {
    DS_INIT_ALLOCATOR(&allocator, memory, size);
}

void *util_malloc(unsigned long size) {
    return DS_MALLOC(&allocator, size);
}

void util_free(void *ptr) {
    DS_FREE(&allocator, ptr);
}
