#ifndef CHESS_H
#define CHESS_H

#include "ds.h"

#define CHESS_WIDTH 8
#define CHESS_HEIGHT 8

typedef char chess_board_t[CHESS_HEIGHT * CHESS_WIDTH];

typedef struct square_t {
    int rank, file;
} square_t;

#define MK_SQUARE(r, f)                                                        \
    (square_t) { .rank = r, .file = f }

typedef struct chess_state_t {
    chess_board_t board;
    int last_move; // 1 if we have a last move
    square_t last_move_start;
    square_t last_move_end;
    char king_moved; // CHESS_WHITE if white moved CHESS_BLACK if black moved
    char short_rook_moved;
    char long_rook_moved;
} chess_state_t;

#define CHESS_NONE 0

#define CHESS_MOVE 1
#define CHESS_ENPASSANT 4
#define CHESS_CASTLE_SHORT 8
#define CHESS_CASTLE_LONG 16

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

#define SQUARE_DARK 0xA97C6D
#define SQUARE_LIGHT 0xF4D3B2
#define SQUARE_DARK_MOVE 0x794C3D
#define SQUARE_LIGHT_MOVE 0xC4A382
#define SQUARE_MOVE 0xFF0000

char chess_square_get(chess_board_t *board, square_t square);
void chess_square_set(chess_board_t *board, square_t square, char piece);

#define CHESS_START "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

void chess_init_fen(chess_state_t *state, ds_string_slice fen);
void chess_apply_move(chess_state_t *state, square_t start, square_t end, char move);
void chess_valid_moves(chess_state_t *state, square_t start, chess_board_t *moves);

int chess_is_in_check(chess_state_t *state, char current);
int chess_controls(chess_state_t *state, square_t target, char current);

char chess_flip_player(char current);
int chess_count_positions(chess_state_t *state, char current, int depth);

#endif // CHESS_H
