#ifndef CHESS_H
#define CHESS_H

#include "ds.h"

typedef struct square_t {
    int rank, file;
} square_t;

#define CHESS_NONE 0
#define CHESS_PAWN 1
#define CHESS_ROOK 2
#define CHESS_KNIGHT 3
#define CHESS_BISHOP 4
#define CHESS_QUEEN 5
#define CHESS_KING 6
#define PIECE_FLAG 0b00111

#define CHESS_WHITE 8
#define CHESS_BLACK 16
#define COLOR_FLAG 0b11000

#define CHESS_WIDTH 8
#define CHESS_HEIGHT 8

typedef char chess_board_t[CHESS_HEIGHT * CHESS_WIDTH];

#define SQUARE_DARK 0xA97C6D
#define SQUARE_LIGHT 0xF4D3B2
#define SQUARE_MOVE 0xFF0000

#define CHESS_START "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

void chess_init_fen(chess_board_t *board, ds_string_slice fen);
char chess_square_get(chess_board_t *board, square_t square);
void chess_square_set(chess_board_t *board, square_t square, char piece);
void chess_valid_moves(chess_board_t *board, square_t square, ds_dynamic_array *moves);
void chess_apply_move(chess_board_t *board, square_t square, square_t selected_square, ds_dynamic_array moves);

#endif // CHESS_H
