#define DS_NO_STDLIB
#define DS_LIST_ALLOCATOR_IMPLEMENTATION
#define DS_DA_INIT_CAPACITY 8
#define DS_DA_IMPLEMENTATION
#define DS_SB_IMPLEMENTATION
#define DS_NO_TERMINAL_COLORS
#include "ds.h"
#include "wasm.h"

#define EMPTY 0
#define BishopDark 1
#define BishopLight 2
#define KingDark 3
#define KingLight 4
#define KnightDark 5
#define KnightLight 6
#define PawnDark 7
#define PawnLight 8
#define QueenDark 9
#define QueenLight 10
#define RookDark 11
#define RookLight 12

#define CHESS_WIDTH 8
#define CHESS_HEIGHT 8

typedef char chess_board_t[CHESS_HEIGHT][CHESS_WIDTH];

typedef struct square_t {
    int row, col;
} square_t;

#define SQUARE_DARK 0x183318
#define SQUARE_LIGHT 0xFFFFFF

DS_ALLOCATOR allocator;
chess_board_t board;
square_t select = {-1};

static void chess_square_set(chess_board_t *board, int row, int col, char piece) {
    (*board)[row][col] = piece;
}

static int square_valid(square_t square) {
    return square.row != -1 && square.col != -1;
}

static void px_to_square(point_t point, square_t *square) {
    if (point.x == -1 || point.y == -1) {
        square->col = -1;
        square->row = -1;
        return;
    }

    int canvas_width = js_width();
    int canvas_height = js_height();

    int cell_width = canvas_width / CHESS_WIDTH;
    int cell_height = canvas_height / CHESS_HEIGHT;

    square->col = point.x / cell_width;
    square->row = (canvas_height - point.y) / cell_height;
}

void chess_reset_board(chess_board_t *board) {
    chess_square_set(board, 0, 0, RookLight);
    chess_square_set(board, 0, 1, KnightLight);
    chess_square_set(board, 0, 2, BishopLight);
    chess_square_set(board, 0, 3, QueenLight);
    chess_square_set(board, 0, 4, KingLight);
    chess_square_set(board, 0, 5, BishopLight);
    chess_square_set(board, 0, 6, KnightLight);
    chess_square_set(board, 0, 7, RookLight);

    for (int i = 0; i < 8; i++) {
        chess_square_set(board, 1, i, PawnLight);
    }

    for (int i = 0; i < 8; i++) {
        chess_square_set(board, 6, i, PawnDark);
    }

    chess_square_set(board, 7, 0, RookDark);
    chess_square_set(board, 7, 1, KnightDark);
    chess_square_set(board, 7, 2, BishopDark);
    chess_square_set(board, 7, 3, QueenDark);
    chess_square_set(board, 7, 4, KingDark);
    chess_square_set(board, 7, 5, BishopDark);
    chess_square_set(board, 7, 6, KnightDark);
    chess_square_set(board, 7, 7, RookDark);
}

static int chess_square_color(int row, int col) {
    int color = 0x000000;
    if (row % 2 == 1) {
        if (col % 2 == 0) {
            color = SQUARE_LIGHT;
        } else {
            color = SQUARE_DARK;
        }
    } else {
        if (col % 2 == 0) {
            color = SQUARE_DARK;
        } else {
            color = SQUARE_LIGHT;
        }
    }
    return color;
}

void chess_print_board(chess_board_t board) {
    int canvas_width = js_width();
    int canvas_height = js_height();

    int cell_width = canvas_width / CHESS_WIDTH;
    int cell_height = canvas_height / CHESS_HEIGHT;

    for (int row = 0; row < CHESS_HEIGHT; row++) {
        for (int col = 0; col < CHESS_WIDTH; col++) {
            int col_px = col * cell_width;
            int row_px = (CHESS_HEIGHT - row - 1) * cell_height;

            int color = chess_square_color(row, col);
            js_fill_rect(col_px, row_px, cell_width, cell_height, color);

            if (board[row][col] != EMPTY) {
                js_fill_piece(col_px, row_px, cell_width, cell_height, board[row][col]);
            }
        }
    }
}

void chess_apply_move(chess_board_t *board, square_t select, square_t target) {
    char piece = (*board)[select.row][select.col];
    chess_square_set(board, select.row, select.col, EMPTY);
    chess_square_set(board, target.row, target.col, piece);
}

void init(void *memory, unsigned long size) {
    js_clear_canvas();

    DS_INIT_ALLOCATOR(&allocator, memory, size);

    ds_string_builder sb = {0};
    ds_string_builder_init_allocator(&sb, &allocator);
    ds_string_builder_append(&sb, "hello, %s %d", "world", 69);
    char *buffer = NULL;
    ds_string_builder_build(&sb, &buffer);
    js_log_cstr(buffer);
    DS_FREE(&allocator, buffer);
    ds_string_builder_free(&sb);

    chess_reset_board(&board);
    chess_print_board(board);
}

void tick(float deltaTime) {
    js_clear_canvas();

    int canvas_width = js_width();
    int canvas_height = js_height();

    int cell_width = canvas_width / CHESS_WIDTH;
    int cell_height = canvas_height / CHESS_HEIGHT;

    point_t point = {0};
    square_t square = {0};
    js_canvas_hover_px(&point);
    px_to_square(point, &square);

    if (square_valid(square) && js_canvas_clicked()) {
        if (!square_valid(select) && board[square.row][square.col] != EMPTY) {
            select = square;
        } else if (square_valid(select)) {
            chess_apply_move(&board, select, square);
            select.row = -1;
            select.col = -1;
        }
    }

    chess_print_board(board);
    if (square_valid(square)) {
        int col_px = square.col * cell_width;
        int row_px = (CHESS_HEIGHT - square.row - 1) * cell_height;
        js_draw_outline(col_px, row_px, cell_width, cell_height, 0x0000FF);
    }
}
