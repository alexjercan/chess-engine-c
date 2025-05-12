#ifdef __wasm__
#include "wasm.h"
#else
#include "raylib.h"
#endif

#include "chess.h"
#include "util.h"

static DS_ALLOCATOR allocator;
static ds_hashmap textures = {0};
static int is_selected = 0;
static int promotion_gui = 0;
static square_t selected_square = {0};
static square_t promotion_square = {0};
static chess_board_t moves = {0};
static char options[4] = { CHESS_QUEEN, CHESS_ROOK, CHESS_BISHOP, CHESS_KNIGHT };

static int px_to_option(Vector2 *px) {
    int col_px = SCREEN_WIDTH / 2 - PROMOTION_GUI_WIDTH / 2;
    int row_px = SCREEN_HEIGHT / 2 - PROMOTION_GUI_HEIGHT / 2;

    if (px->x < col_px || px->x > col_px + PROMOTION_GUI_WIDTH || px->y < row_px || px->y > row_px + PROMOTION_GUI_HEIGHT) {
        return -1;
    } else {
        return (px->x - col_px) / PROMOTION_GUI_HEIGHT;
    }
}

static void chess_print_promotion(const chess_state_t *state) {
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
        char piece = option | state->current_player;

        int col_px = SCREEN_WIDTH / 2 - PROMOTION_GUI_WIDTH / 2 + i * PROMOTION_GUI_HEIGHT;
        int row_px = SCREEN_HEIGHT / 2 - PROMOTION_GUI_HEIGHT / 2;

        Texture2D texture = LoadTextureCached(&textures, chess_piece_texture_path(piece));
        float scale = (float)PROMOTION_GUI_HEIGHT / texture.width;
        DrawTextureEx(texture, (Vector2){.x = col_px, .y = row_px}, 0, scale, WHITE);
    }
}

void chess_init(void *memory, unsigned long size) {
    DS_INIT_ALLOCATOR(&allocator, memory, size);

    if (ds_hashmap_init_allocator(&textures, MAX_CAPACITY, string_hash, string_compare, &allocator) != DS_OK) {
        DS_PANIC("Error initializing hashmap");
    }
}

void chess_move(chess_state_t *state) {
    int cell_width = SCREEN_WIDTH / CHESS_WIDTH;
    int cell_height = SCREEN_HEIGHT / CHESS_HEIGHT;

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

    if (promotion_gui == 1) {
        chess_print_promotion(state);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse_px = GetMousePosition();

        if (promotion_gui == 0) {
            square_t square = {0};
            px_to_square(&mouse_px, &square);

            if ((chess_square_get(&state->board, square) & COLOR_FLAG) == state->current_player) {
                is_selected = 1;
                selected_square = square;
                chess_valid_moves(state, square, &moves);
            } else {
                is_selected = 0;
                char move = chess_square_get(&moves, square);
                if (move != CHESS_NONE) {
                    chess_apply_move(state, selected_square, square, move);
                    if ((move & CHESS_PROMOTE) != 0) {
                        promotion_gui = 1;
                        promotion_square = square;
                    } else {
                        state->current_player = chess_flip_player(state->current_player);
                    }
                }
                DS_MEMSET(&moves, 0, sizeof(chess_board_t));
            }
        } else if (promotion_gui == 1) {
            int option_index = px_to_option(&mouse_px);
            if (option_index >= 0) {
                char option = options[option_index];
                char piece = option | state->current_player;
                chess_square_set(&state->board, promotion_square, piece);
                state->current_player = chess_flip_player(state->current_player);
                promotion_gui = 0;
            }
        }
    }
}
