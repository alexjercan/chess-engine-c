#ifdef __wasm__
#define DS_LIST_ALLOCATOR_IMPLEMENTATION
#include "wasm.h"
#else
#include "raylib.h"
#endif

#include "chess.h"
#include "game.h"

#ifndef ASSETS_FOLDER
#define ASSETS_FOLDER "dist/assets/"
#endif

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800

#define PROMOTION_GUI_HEIGHT 150
#define PROMOTION_GUI_WIDTH 4 * PROMOTION_GUI_HEIGHT

#define MAX_CAPACITY 100

DS_ALLOCATOR allocator;
ds_hashmap textures = {0};
chess_state_t state = {0};
chess_board_t moves = {0};
int is_selected = 0;
square_t selected_square = {0};
char current_player = CHESS_WHITE;
int promotion_gui = 0;
square_t promotion_square = {0};
char options[4] = { CHESS_QUEEN, CHESS_ROOK, CHESS_BISHOP, CHESS_KNIGHT };
int checkmate_gui = 0;

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
            DS_PANIC("Chess piece is not supported %d", piece);
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

static int px_to_option(Vector2 *px) {
    int col_px = SCREEN_WIDTH / 2 - PROMOTION_GUI_WIDTH / 2;
    int row_px = SCREEN_HEIGHT / 2 - PROMOTION_GUI_HEIGHT / 2;

    if (px->x < col_px || px->x > col_px + PROMOTION_GUI_WIDTH || px->y < row_px || px->y > row_px + PROMOTION_GUI_HEIGHT) {
        return -1;
    } else {
        return (px->x - col_px) / PROMOTION_GUI_HEIGHT;
    }
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

static void chess_print_board() {
    int cell_width = SCREEN_WIDTH / CHESS_WIDTH;
    int cell_height = SCREEN_HEIGHT / CHESS_HEIGHT;

    square_t king_square = {0};
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

            square_t square = (square_t){.rank = rank, .file = file};
            char piece = chess_square_get(&state.board, square);
            char piece_color = piece & COLOR_FLAG;
            char piece_type = piece & PIECE_FLAG;
            if (piece_color == current_player && piece_type == CHESS_KING) {
                king_square = square;
            }
        }
    }

    if (chess_is_in_check(&state, current_player)) {
        int file = king_square.file;
        int rank = king_square.rank;

        int square_color = SQUARE_MOVE;
        Color color = {.r = (square_color & 0xFF0000) >> 16,
                       .g = (square_color & 0x00FF00) >> 8,
                       .b = (square_color & 0x0000FF) >> 0,
                       .a = 0xFF};

        int file_px = file * cell_width;
        int rank_px = (CHESS_WIDTH - rank - 1) * cell_height;

        DrawRectangle(file_px, rank_px, cell_width, cell_height, color);
    }

    if (state.last_move) {
        int file = state.last_move_start.file;
        int rank = state.last_move_start.rank;

        int file_px = file * cell_width;
        int rank_px = (CHESS_WIDTH - rank - 1) * cell_height;
        int is_light_square = (file + rank) % 2 == 1;
        int square_color = is_light_square ? SQUARE_LIGHT_MOVE : SQUARE_DARK_MOVE;
        Color color = {.r = (square_color & 0xFF0000) >> 16,
                       .g = (square_color & 0x00FF00) >> 8,
                       .b = (square_color & 0x0000FF) >> 0,
                       .a = 0xFF};

        DrawRectangle(file_px, rank_px, cell_width, cell_height, color);
    }

    if (state.last_move) {
        int file = state.last_move_end.file;
        int rank = state.last_move_end.rank;

        int file_px = file * cell_width;
        int rank_px = (CHESS_WIDTH - rank - 1) * cell_height;
        int is_light_square = (file + rank) % 2 == 1;
        int square_color = is_light_square ? SQUARE_LIGHT_MOVE : SQUARE_DARK_MOVE;
        Color color = {.r = (square_color & 0xFF0000) >> 16,
                       .g = (square_color & 0x00FF00) >> 8,
                       .b = (square_color & 0x0000FF) >> 0,
                       .a = 0xFF};

        DrawRectangle(file_px, rank_px, cell_width, cell_height, color);
    }

    for (unsigned int file = 0; file < CHESS_WIDTH; file++) {
        for (unsigned int rank = 0; rank < CHESS_HEIGHT; rank++) {
            int file_px = file * cell_width;
            int rank_px = (CHESS_WIDTH - rank - 1) * cell_height;

            square_t square = (square_t){.rank = rank, .file = file};
            char piece = chess_square_get(&state.board, square);
            if (piece != CHESS_NONE) {
                Texture2D texture = LoadTextureCached(chess_piece_texture_path(piece));
                float scale = (float)cell_width / texture.width;
                DrawTextureEx(texture, (Vector2){.x = file_px, .y = rank_px}, 0, scale, WHITE);
            }
        }
    }

    for (unsigned int file = 0; file < CHESS_WIDTH; file++) {
        for (unsigned int rank = 0; rank < CHESS_HEIGHT; rank++) {
            square_t end = (square_t){.rank = rank, .file = file};
            char move = chess_square_get(&moves, end);

            if (move != CHESS_NONE) {
                Vector2 px = {0};
                square_to_px(&end, &px);

                Color color = {.r = (SQUARE_MOVE & 0xFF0000) >> 16,
                               .g = (SQUARE_MOVE & 0x00FF00) >> 8,
                               .b = (SQUARE_MOVE & 0x0000FF) >> 0,
                               .a = 0xFF};
                DrawCircle(px.x + cell_width / 2.0f, px.y + cell_height / 2.0f, cell_width / 4.0f, color);
            }
        }
    }
}

static void chess_print_promotion() {
    int col_px = SCREEN_WIDTH / 2 - PROMOTION_GUI_WIDTH / 2;
    int row_px = SCREEN_HEIGHT / 2 - PROMOTION_GUI_HEIGHT / 2;

    int is_light_square = (promotion_square.file + promotion_square.rank) % 2 == 1;
    int square_color = is_light_square ? SQUARE_LIGHT : SQUARE_DARK;
    Color color = {.r = (square_color & 0xFF0000) >> 16,
                   .g = (square_color & 0x00FF00) >> 8,
                   .b = (square_color & 0x0000FF) >> 0,
                   .a = 0xFF};

    DrawRectangle(col_px, row_px, PROMOTION_GUI_WIDTH, PROMOTION_GUI_HEIGHT, color);

    for (unsigned int i = 0; i < 4; i++) {
        char option = options[i];
        char piece = option | current_player;

        int col_px = SCREEN_WIDTH / 2 - PROMOTION_GUI_WIDTH / 2 + i * PROMOTION_GUI_HEIGHT;
        int row_px = SCREEN_HEIGHT / 2 - PROMOTION_GUI_HEIGHT / 2;

        Texture2D texture = LoadTextureCached(chess_piece_texture_path(piece));
        float scale = (float)PROMOTION_GUI_HEIGHT / texture.width;
        DrawTextureEx(texture, (Vector2){.x = col_px, .y = row_px}, 0, scale, WHITE);
    }
}

static void chess_print_checkmate() {
    int col_px = SCREEN_WIDTH / 2 - PROMOTION_GUI_WIDTH / 2;
    int row_px = SCREEN_HEIGHT / 2 - PROMOTION_GUI_HEIGHT / 2;

    Color color = {.r = (0xFF0000 & 0xFF0000) >> 16,
                   .g = (0xFF0000 & 0x00FF00) >> 8,
                   .b = (0xFF0000 & 0x0000FF) >> 0,
                   .a = 0xFF};

    DrawRectangle(col_px, row_px, PROMOTION_GUI_WIDTH, PROMOTION_GUI_HEIGHT, color);

    const char *text = "Checkmate!";
    int text_width = MeasureText(text, 20);
    int text_height = 20;
    int text_x = col_px + PROMOTION_GUI_WIDTH / 2 - text_width / 2;
    int text_y = row_px + PROMOTION_GUI_HEIGHT / 2 - text_height / 2;

    DrawText(text, text_x, text_y, 20, WHITE);
}

void init(void *memory, unsigned long size) {
    DS_INIT_ALLOCATOR(&allocator, memory, size);

    if (ds_hashmap_init_allocator(&textures, MAX_CAPACITY, string_hash, string_compare, &allocator) != DS_OK) {
        DS_PANIC("Error initializing hashmap");
    }

    DS_MEMSET(&moves, 0, sizeof(chess_board_t));

    ds_string_slice fen = DS_STRING_SLICE(CHESS_START);
    chess_init_fen(&state, fen);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chess Engine");
}

void tick(float deltaTime) {
    UNUSED(deltaTime);

    BeginDrawing();

    ClearBackground(RAYWHITE);
    chess_print_board();
    if (promotion_gui == 1) {
        chess_print_promotion();
    }
    if (checkmate_gui == 1) {
        chess_print_checkmate();
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse_px = GetMousePosition();

        if (promotion_gui == 0 && checkmate_gui == 0) {
            square_t square = {0};
            px_to_square(&mouse_px, &square);

            if ((chess_square_get(&state.board, square) & COLOR_FLAG) == current_player) {
                is_selected = 1;
                selected_square = square;
                chess_valid_moves(&state, square, &moves);
            } else {
                is_selected = 0;
                char move = chess_square_get(&moves, square);
                if (move != CHESS_NONE) {
                    chess_apply_move(&state, selected_square, square, move);
                    if ((move & CHESS_PROMOTE) != 0) {
                        promotion_gui = 1;
                        promotion_square = square;
                    } else {
                        current_player = chess_flip_player(current_player);
                        if (chess_is_checkmate(&state, current_player)) {
                            checkmate_gui = 1;
                        }
                    }
                }
                DS_MEMSET(&moves, 0, sizeof(chess_board_t));
            }
        } else if (promotion_gui == 1) {
            int option_index = px_to_option(&mouse_px);
            if (option_index >= 0) {
                char option = options[option_index];
                char piece = option | current_player;
                chess_square_set(&state.board, promotion_square, piece);
                current_player = chess_flip_player(current_player);
                if (chess_is_checkmate(&state, current_player)) {
                    checkmate_gui = 1;
                }
                promotion_gui = 0;
            }
        }
    }

    EndDrawing();
}
