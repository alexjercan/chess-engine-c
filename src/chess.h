#ifndef CHESS_H
#define CHESS_H

#include "ds.h"

typedef struct square_t {
    int rank, file;
} square_t;

#define MK_SQUARE(r, f)                                                        \
    (square_t) { .rank = r, .file = f }

typedef struct move_t {
    int is_move;
    square_t start, end;
    int takes;
    square_t takes_square;
} move_t;

#define MK_MOVE_TAKES(start_, end_, takes_square_)                             \
  (move_t) {                                                                   \
    .is_move = 1, .start = start_, .end = end_, .takes = 1,                    \
    .takes_square = takes_square_                                              \
  }

#define MK_MOVE(start_, end_)                                                  \
  (move_t) {                                                                   \
    .is_move = 1, .start = start_, .end = end_, .takes = 0,                    \
    .takes_square = MK_SQUARE(0, 0)                                            \
  }

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
#define SQUARE_DARK_MOVE 0x794C3D
#define SQUARE_LIGHT_MOVE 0xC4A382
#define SQUARE_MOVE 0xFF0000

#define CHESS_START "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

void chess_init_fen(chess_board_t *board, ds_string_slice fen);
char chess_square_get(chess_board_t *board, square_t square);
void chess_square_set(chess_board_t *board, square_t square, char piece);
void chess_apply_move(chess_board_t *board, move_t move);

void chess_valid_moves(chess_board_t *board, square_t square, move_t last_move, ds_dynamic_array *moves /* move_t */);
int chess_is_in_check(chess_board_t *board, char current, DS_ALLOCATOR *allocator);


char chess_flip_player(char current);
int chess_count_positions(chess_board_t *board, move_t last_move, char current, int depth, DS_ALLOCATOR *allocator);

#endif // CHESS_H
