#include "util.h"

DS_ALLOCATOR allocator = {0};
static ds_hashmap textures = {0};
static ds_hashmap sounds = {0};

static const char *chess_piece_texture_path(char piece) {
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

static const char *chess_move_sound_path(char move) {
    switch (move) {
        case CHESS_MOVE:
            return ASSETS_FOLDER "move.ogg";
        case CHESS_CAPTURE:
            return ASSETS_FOLDER "capture.ogg";
        case CHESS_CASTLE_SHORT:
            return ASSETS_FOLDER "castle.ogg";
        case CHESS_CASTLE_LONG:
            return ASSETS_FOLDER "castle.ogg";
        default:
            DS_PANIC("Chess move is not supported %d", move);
            return NULL;
    }
}

static Texture2D LoadTextureCachedMap(ds_hashmap *textures, char piece) {
    Texture2D texture = {0};
    long key = piece;
    ds_hashmap_kv kv = { .key = (void *)key, .value = NULL };
    DS_UNREACHABLE(ds_hashmap_get_or_default(textures, &kv, NULL));

    if (kv.value != NULL) {
        DS_MEMCPY(&texture, kv.value, sizeof(Texture2D));
        return texture;
    }

    texture = LoadTexture(chess_piece_texture_path(piece));
    kv.value = DS_MALLOC(textures->allocator, sizeof(Texture2D));
    DS_MEMCPY(kv.value, &texture, sizeof(Texture2D));
    ds_hashmap_insert(textures, &kv);

    return texture;
}

static Sound LoadSoundCachedMap(ds_hashmap *sounds, char move) {
    Sound sound = {0};
    long key = move;
    ds_hashmap_kv kv = { .key = (void *)key, .value = NULL };
    DS_UNREACHABLE(ds_hashmap_get_or_default(sounds, &kv, NULL));

    if (kv.value != NULL) {
        DS_MEMCPY(&sound, kv.value, sizeof(Sound));
        return sound;
    }

    sound = LoadSound(chess_move_sound_path(move));
    kv.value = DS_MALLOC(sounds->allocator, sizeof(Sound));
    DS_MEMCPY(kv.value, &sound, sizeof(Sound));
    ds_hashmap_insert(sounds, &kv);

    return sound;
}

Texture2D LoadTextureCachedPiece(char piece) {
    if (textures.capacity == 0) {
        if (ds_hashmap_init_allocator(&textures, MAX_CAPACITY, long_hash, long_compare, &allocator) != DS_OK) {
            DS_PANIC("Error initializing hashmap");
        }
    }

    return LoadTextureCachedMap(&textures, piece);
}

Sound LoadSoundCachedMove(char move) {
    if (sounds.capacity == 0) {
        if (ds_hashmap_init_allocator(&sounds, MAX_CAPACITY, long_hash, long_compare, &allocator) != DS_OK) {
            DS_PANIC("Error initializing hashmap");
        }
    }

    return LoadSoundCachedMap(&sounds, move);
}

unsigned long long_hash(const void *key) {
    return (long)key % MAX_CAPACITY;
}

int long_compare(const void *k1, const void *k2) {
    return (long)k1 - (long)k2;
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

move_score minmax(const chess_state_t *state, move_t *choices, int count,
                  char maxxing, int depth, int alpha, int beta, eval_fn *eval,
                  sort_fn *sort, minmax_info *info) {
    char result = chess_checkmate(state);
    if (result != CHESS_NONE) {
        info->positions += 1;
        return MK_MOVE_SCORE(-1, (maxxing == result) ? -MINMAX_INF : MINMAX_INF);
    }

    if (chess_draw(state)) {
        info->positions += 1;
        return MK_MOVE_SCORE(-1, 0);
    }

    if (depth == 0) {
        info->positions += 1;
        return MK_MOVE_SCORE(-1, eval(state, maxxing));
    }

    move_score best = {.score = 0, .move = -1};
    if (maxxing == state->current_player) best.score = -MINMAX_INF;
    else best.score = MINMAX_INF;
    best.move = (count == 0) ? -1 : rand() % count;

    ds_dynamic_array moves = {0}; /* move_t */
    ds_dynamic_array_init_allocator(&moves, sizeof(move_t), &allocator);
    for (int i = 0; i < count; i++) {
        chess_state_t clone = {0};
        DS_MEMCPY(&clone, state, sizeof(chess_state_t));

        move_t move = choices[i];
        chess_apply_move(&clone, move);

        clone.current_player = chess_flip_player(clone.current_player);

        chess_generate_moves(&clone, &moves);
        if (sort != NULL) ds_dynamic_array_sort(&moves, sort);

        move_score value = minmax(&clone, moves.items, moves.count, maxxing, depth - 1, alpha, beta, eval, sort, info);

        if (maxxing == state->current_player) {
            if (value.score > best.score) {
                best.score = value.score;
                best.move = i;
            }

            if (value.score > alpha) alpha = value.score;
        } else {
            if (value.score < best.score) {
                best.score = value.score;
                best.move = i;
            }

            if (value.score < beta) beta = value.score;
        }

        if (alpha > beta) break;
    }

    ds_dynamic_array_free(&moves);

    return best;
}
