#include "chess.h"
#include "game.h"
#include "util.h"

static DS_ALLOCATOR allocator;
static ds_hashmap textures = {0};
static chess_state_t state = {0};
static int checkmate_gui = 0;

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

    if (chess_is_in_check(&state, state.current_player)) {
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
                Texture2D texture = LoadTextureCached(&textures, chess_piece_texture_path(piece));
                float scale = (float)cell_width / texture.width;
                DrawTextureEx(texture, (Vector2){.x = file_px, .y = rank_px}, 0, scale, WHITE);
            }
        }
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

    ds_string_slice fen = DS_STRING_SLICE(CHESS_START);
    chess_init_fen(&state, fen);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chess Engine");
}

void tick(float deltaTime) {
    UNUSED(deltaTime);

    BeginDrawing();

    ClearBackground(RAYWHITE);
    chess_print_board();
    if (checkmate_gui == 1) {
        chess_print_checkmate();
    }

    if (chess_is_checkmate(&state, state.current_player)) {
        checkmate_gui = 1;
    } else {
        if (state.current_player == CHESS_WHITE) {
            move_player1_fn(&state);
        } else {
            move_player2_fn(&state);
        }
    }

    EndDrawing();
}
