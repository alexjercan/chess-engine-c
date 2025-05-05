#ifdef DS_NO_STDLIB
#define DS_LIST_ALLOCATOR_IMPLEMENTATION
#include "wasm.h"
#else
#include "raylib.h"
#endif // DS_NO_STDLIB

#include "chess.h"
#include "game.h"

#ifndef ASSETS_FOLDER
#define ASSETS_FOLDER "dist/assets/"
#endif

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800

#define MAX_CAPACITY 100

DS_ALLOCATOR allocator;
ds_hashmap textures = {0};
chess_board_t board;
square_t selected_square = {0};
int is_selected = 0;
ds_dynamic_array moves = {0};

static unsigned long string_hash(const void *key) {
    unsigned long hash = 0;
    char *name = (char *)key;
    for (unsigned int i = 0; i < DS_STRLEN(name); i++) {
        hash = 31 * hash + name[i];
    }
    return hash % MAX_CAPACITY;
}

static int string_compare(const void *k1, const void *k2) {
    return DS_STRCMP((char *)k1, (char *)k2);
}

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
            return NULL;
    }
}

static void px_to_square(Vector2 *px, square_t *square) {
    int cell_width = SCREEN_WIDTH / CHESS_WIDTH;
    int cell_height = SCREEN_HEIGHT / CHESS_HEIGHT;

    square->file = (int)px->x / cell_width;
    square->rank = (CHESS_HEIGHT - (int)px->y / cell_height - 1);
}

static void square_to_px(square_t *square, Vector2 *px) {
    int cell_width = SCREEN_WIDTH / CHESS_WIDTH;
    int cell_height = SCREEN_HEIGHT / CHESS_HEIGHT;

    px->x = square->file * cell_width;
    px->y = (CHESS_WIDTH - square->rank - 1) * cell_height;
}

static Texture2D LoadTextureCached(const char *fileName) {
    Texture2D texture = {0};
    ds_hashmap_kv kv = { .key = (void *)fileName, .value = NULL };
    DS_UNREACHABLE(ds_hashmap_get_or_default(&textures, &kv, NULL));

    if (kv.value != NULL) {
        DS_MEMCPY(&texture, kv.value, sizeof(Texture2D));
        return texture;
    }

    texture = LoadTexture(fileName);
    kv.value = DS_MALLOC(textures.allocator, sizeof(Texture2D));
    DS_MEMCPY(kv.value, &texture, sizeof(Texture2D));
    ds_hashmap_insert(&textures, &kv);

    return texture;
}

static void chess_print_board(chess_board_t *board) {
    int cell_width = SCREEN_WIDTH / CHESS_WIDTH;
    int cell_height = SCREEN_HEIGHT / CHESS_HEIGHT;

    for (unsigned int file = 0; file < CHESS_WIDTH; file++) {
        for (unsigned int rank = 0; rank < CHESS_HEIGHT; rank++) {
            int is_light_square = (file + rank) % 2 == 1;
            int square_color = is_light_square ? SQUARE_LIGHT : SQUARE_DARK;
            Color color = {.r = (square_color & 0xFF0000) >> 16,
                           .g = (square_color & 0x00FF00) >> 8,
                           .b = (square_color & 0x0000FF) >> 0,
                           .a = 0xFF};

            int file_px = file * cell_width;
            int rank_px = (CHESS_WIDTH - rank - 1) * cell_height;

            DrawRectangle(file_px, rank_px, cell_width, cell_height, color);
            char piece = chess_square_get(board, (square_t){.rank = rank, .file = file});
            if (piece != CHESS_NONE) {
                Texture2D texture = LoadTextureCached(chess_piece_texture_path(piece));
                float scale = (float)cell_width / texture.width;
                DrawTextureEx(texture, (Vector2){.x = file_px, .y = rank_px}, 0, scale, WHITE);
            }
        }
    }

    for (unsigned int i = 0; i < moves.count; i++) {
        square_t *square = NULL;
        DS_UNREACHABLE(ds_dynamic_array_get_ref(&moves, i, (void **)&square));

        Vector2 px = {0};
        square_to_px(square, &px);

        Color color = {.r = (SQUARE_MOVE & 0xFF0000) >> 16,
                       .g = (SQUARE_MOVE & 0x00FF00) >> 8,
                       .b = (SQUARE_MOVE & 0x0000FF) >> 0,
                       .a = 0xFF};
        DrawCircle(px.x + cell_width / 2.0f, px.y + cell_height / 2.0f, cell_width / 4.0f, color);
    }
}

void init(void *memory, unsigned long size) {
    DS_INIT_ALLOCATOR(&allocator, memory, size);

    if (ds_hashmap_init_allocator(&textures, MAX_CAPACITY, string_hash, string_compare, &allocator) != DS_OK) {
        DS_PANIC("Error initializing hashmap");
    }

    ds_dynamic_array_init_allocator(&moves, sizeof(Vector2), &allocator);

    ds_string_slice fen = DS_STRING_SLICE(CHESS_START);
    chess_init_fen(&board, fen);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chess Engine");
}

void tick(float deltaTime) {
    UNUSED(deltaTime);

    BeginDrawing();

    ClearBackground(RAYWHITE);
    chess_print_board(&board);

    Vector2 mouse_px = GetMousePosition();
    square_t square = {0};
    px_to_square(&mouse_px, &square);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (is_selected) {
            is_selected = 0;
            chess_apply_move(&board, square, selected_square, moves);
            ds_dynamic_array_clear(&moves);
        } else if (chess_square_get(&board, square) != CHESS_NONE) {
            is_selected = 1;
            selected_square = square;
            chess_valid_moves(&board, square, &moves);
        }
    }

    EndDrawing();
}
