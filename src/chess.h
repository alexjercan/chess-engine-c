#ifndef CHESS_H
#define CHESS_H

#include "ds.h"

#define CHESS_WIDTH 8
#define CHESS_HEIGHT 8

#define CHESS_PROMOTE_OPTIONS (char[4]){ CHESS_QUEEN, CHESS_ROOK, CHESS_BISHOP, CHESS_KNIGHT }

typedef char chess_board_t[CHESS_HEIGHT * CHESS_WIDTH];

typedef struct square_t {
    int rank, file;
} square_t;

#define MK_SQUARE(r, f)                                                        \
    (square_t) { .rank = r, .file = f }

typedef struct move_t {
    square_t start;
    square_t end;
    char move;
    char promotion; // In case we have a promote move we have to choose a piece
} move_t;

#define MK_MOVE(s, e, m, p)                                                  \
    (move_t) { .start = s, .end = e, .move = m, .promotion = p }

typedef struct chess_state_t {
    chess_board_t board;
    int last_move; // 1 if we have a last move
    square_t last_move_start;
    square_t last_move_end;
    char king_moved; // CHESS_WHITE if white moved CHESS_BLACK if black moved
    char short_rook_moved;
    char long_rook_moved;

    char current_player;
} chess_state_t;

unsigned long chess_state_size(void);
unsigned long chess_move_size(void);

#define CHESS_NONE 0

#define CHESS_MOVE 1
#define CHESS_PROMOTE 2
#define CHESS_ENPASSANT 4
#define CHESS_CASTLE_SHORT 8
#define CHESS_CASTLE_LONG 16
#define CHESS_CAPTURE 32
#define CHESS_CHECK 64

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

#define EVAL_PAWN 100
#define EVAL_KNIGHT 300
#define EVAL_BISHOP 300
#define EVAL_ROOK 500
#define EVAL_QUEEN 900
#define EVAL_KING 100

#define SQUARE_DARK 0xA97C6D
#define SQUARE_LIGHT 0xF4D3B2
#define SQUARE_DARK_MOVE 0x794C3D
#define SQUARE_LIGHT_MOVE 0xC4A382
#define SQUARE_MOVE 0xFF0000

char chess_square_get(const chess_board_t *board, square_t square);
void chess_square_set(chess_board_t *board, square_t square, char piece);
boolean chess_move_get(const move_t *moves, int count, move_t filter, int *index);

#define CHESS_START "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "

void chess_init_fen(chess_state_t *state, ds_string_slice fen);
void chess_dump_fen(const chess_state_t *state, char **fen);
void chess_apply_move(chess_state_t *state, move_t move);
void chess_generate_moves(const chess_state_t *state, ds_dynamic_array *moves /* move_t */);
char chess_flip_player(char current);

// Functions to check if the game is over
char chess_checkmate(const chess_state_t *state);
int chess_draw(const chess_state_t *state);

int chess_is_in_check(const chess_state_t *state, char current);
int chess_is_checkmate(const chess_state_t *state, char current);
int chess_is_stalemate(const chess_state_t *state, char current);
int chess_is_draw(const chess_state_t *state, char current);

int chess_count_material(const chess_state_t *state, char current);
int chess_count_material_weighted(const chess_state_t *state, char current);

// Functions to count the number of positions for testing
typedef struct perft_t {
    int nodes;
    int captures;
    int enp;
    int castles;
    int promote;
    int checks;
    int checkmates;
} perft_t;

void chess_count_positions(const chess_state_t *state, int depth, perft_t *perft);

// These are used for the different strategies
void chess_init(void *memory, unsigned long size);
void chess_move(const chess_state_t *state, move_t *moves, int count, int *index);

#endif // CHESS_H
