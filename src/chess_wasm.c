#define DS_NO_STDLIB
#include "wasm.h"
#include "ds.h"
#include "chess.h"

DS_ALLOCATOR allocator;
chess_board_t board;

static void chess_square_set(chess_board_t *board, int row, int col, char piece) {
    (*board)[row][col] = piece;
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
            color = 0xFFFFFF;
        } else {
            color = 0x181818;
        }
    } else {
        if (col % 2 == 0) {
            color = 0x181818;
        } else {
            color = 0xFFFFFF;
        }
    }
    return color;
}

void chess_print_board(chess_board_t board) {
    int canvas_width = js_width();
    int canvas_height = js_height();

    int cell_width = canvas_width / CHESS_WIDTH;
    int cell_height = canvas_height / CHESS_HEIGHT;

    js_clear_canvas();
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

void init(void *memory, unsigned long size) {
    DS_INIT_ALLOCATOR(&allocator, memory, size);

    chess_reset_board(&board);
    chess_print_board(board);
}

void tick(float deltaTime) {
    chess_print_board(board);
}
