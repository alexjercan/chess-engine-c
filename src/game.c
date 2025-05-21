#include "chess.h"
#include "game.h"
#include "util.h"

extern DS_ALLOCATOR allocator;

static chess_state_t state = {0};
static int checkmate_gui = 0;
static int stalemate_gui = 0;
static int draw_gui = 0;
static int is_in_check = 0;

static ds_dynamic_array moves = {0}; /* move_t */

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
            if (piece_color == state.current_player && piece_type == CHESS_KING) {
                king_square = square;
            }
        }
    }

    if (is_in_check) {
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
                Texture2D texture = LoadTextureCachedPiece(piece);
                float scale = (float)cell_width / texture.width;
                DrawTextureEx(texture, (Vector2){.x = file_px, .y = rank_px}, 0, scale, WHITE);
            }
        }
    }
}

static void chess_print_message(const char *text) {
    int col_px = SCREEN_WIDTH / 2 - PROMOTION_GUI_WIDTH / 2;
    int row_px = SCREEN_HEIGHT / 2 - PROMOTION_GUI_HEIGHT / 2;

    Color color = {.r = (0xFF0000 & 0xFF0000) >> 16,
                   .g = (0xFF0000 & 0x00FF00) >> 8,
                   .b = (0xFF0000 & 0x0000FF) >> 0,
                   .a = 0xFF};

    DrawRectangle(col_px, row_px, PROMOTION_GUI_WIDTH, PROMOTION_GUI_HEIGHT, color);

    int text_width = MeasureText(text, 20);
    int text_height = 20;
    int text_x = col_px + PROMOTION_GUI_WIDTH / 2 - text_width / 2;
    int text_y = row_px + PROMOTION_GUI_HEIGHT / 2 - text_height / 2;

    DrawText(text, text_x, text_y, 20, WHITE);
}

void init(void *memory, unsigned long size) {
    util_init(memory, size);

    ds_string_slice fen = DS_STRING_SLICE(CHESS_START);
    chess_init_fen(&state, fen);

    ds_dynamic_array_init_allocator(&moves, sizeof(move_t), &allocator);
    chess_generate_moves(&state, &moves);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chess Engine");

    InitAudioDevice();
}

void tick(float deltaTime) {
    UNUSED(deltaTime);

    BeginDrawing();

    ClearBackground(RAYWHITE);
    chess_print_board();
    if (checkmate_gui == 1) {
        chess_print_message("Checkmate!");
    } else if (stalemate_gui == 1) {
        chess_print_message("Stalemate!");
    } else if (draw_gui == 1) {
        chess_print_message("Draw!");
    } else {
        int index = -1;

        if (state.current_player == CHESS_WHITE) {
            move_player1_fn(&state, moves.items, moves.count, &index);
        } else {
            move_player2_fn(&state, moves.items, moves.count, &index);
        }

        if (index != -1) {
            move_t *move = NULL;
            ds_dynamic_array_get_ref(&moves, index, (void **)&move);
            chess_apply_move(&state, *move);

            if (move->move == CHESS_MOVE) {
                PlaySound(LoadSoundCachedMove(CHESS_MOVE));
            } else if ((move->move & CHESS_CAPTURE) != 0) {
                PlaySound(LoadSoundCachedMove(CHESS_CAPTURE));
            } else if ((move->move & CHESS_CASTLE_SHORT) != 0 || (move->move & CHESS_CASTLE_LONG) != 0) {
                PlaySound(LoadSoundCachedMove(CHESS_CASTLE_SHORT));
            }

            // In case of apply move we just re-draw the board to be safe
            ClearBackground(RAYWHITE);
            chess_print_board();

            state.current_player = chess_flip_player(state.current_player);

            is_in_check = chess_is_in_check(&state, state.current_player);

            chess_generate_moves(&state, &moves);

            if (checkmate_gui == 0 && chess_is_checkmate(&state, state.current_player)) {
                checkmate_gui = 1;
            } else if (stalemate_gui == 0 && chess_is_stalemate(&state, state.current_player)) {
                stalemate_gui = 1;
            } else if (draw_gui == 0 && chess_is_draw(&state, state.current_player)) {
                draw_gui = 1;
            }
        }
    }

    EndDrawing();
}
