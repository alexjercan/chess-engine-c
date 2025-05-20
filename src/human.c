#ifdef __wasm__
#include "wasm.h"
#else
#include "raylib.h"
#endif

#include "chess.h"
#include "util.h"

static int is_selected = 0;
static int promotion_gui = 0;
static square_t selected_square = MK_SQUARE(-1, -1);
static square_t promotion_square = MK_SQUARE(-1, -1);

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
        char option = CHESS_PROMOTE_OPTIONS[i];
        char piece = option | state->current_player;

        int col_px = SCREEN_WIDTH / 2 - PROMOTION_GUI_WIDTH / 2 + i * PROMOTION_GUI_HEIGHT;
        int row_px = SCREEN_HEIGHT / 2 - PROMOTION_GUI_HEIGHT / 2;

        Texture2D texture = LoadTextureCachedPiece(piece);
        float scale = (float)PROMOTION_GUI_HEIGHT / texture.width;
        DrawTextureEx(texture, (Vector2){.x = col_px, .y = row_px}, 0, scale, WHITE);
    }
}

void chess_init(void *memory, unsigned long size) {
    util_init(memory, size);
}

void chess_move(const chess_state_t *state, move_t *choices, int count, int *index) {
    *index = -1;

    int cell_width = SCREEN_WIDTH / CHESS_WIDTH;
    int cell_height = SCREEN_HEIGHT / CHESS_HEIGHT;

    for (int i = 0; i < count; i++) {
        if (choices[i].start.file == selected_square.file && choices[i].start.rank == selected_square.rank) {
            char move = choices[i].move;
            square_t end = choices[i].end;

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
            } else {
                int i = -1;
                is_selected = 0;
                chess_move_get(choices, count, MK_MOVE(selected_square, square, CHESS_NONE, CHESS_NONE), &i);
                if (i != -1) {
                    char move = choices[i].move;
                    if ((move & CHESS_PROMOTE) != 0) {
                        promotion_gui = 1;
                        promotion_square = square;
                    } else {
                        chess_move_get(choices, count, MK_MOVE(selected_square, square, move, CHESS_NONE), index);
                    }
                }
            }
        } else if (promotion_gui == 1) {
            int option_index = px_to_option(&mouse_px);
            if (option_index >= 0) {
                char option = CHESS_PROMOTE_OPTIONS[option_index];
                char piece = option | state->current_player;
                chess_move_get(choices, count, MK_MOVE(selected_square, promotion_square, CHESS_NONE, piece), index);
                promotion_gui = 0;
            }
        }
    }
}
