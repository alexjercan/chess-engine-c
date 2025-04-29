#ifndef CHESS_H
#define CHESS_H

#include "ds.h"

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

void chess_reset_board(chess_board_t *board);
void chess_print_board(chess_board_t board);
void chess_moves(chess_board_t board, square_t select, ds_dynamic_array *squares /* square_t */);
void chess_print_move(chess_board_t board, square_t select, square_t square);
void chess_apply_move(chess_board_t *board, square_t select, square_t target);

#endif // CHESS_H
