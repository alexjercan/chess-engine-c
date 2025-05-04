#define DS_DA_IMPLEMENTATION
#define DS_SB_IMPLEMENTATION
#define DS_HM_IMPLEMENTATION
#include "chess.h"

void chess_init_fen(chess_board_t *board, ds_string_slice fen) {
    unsigned int rank = 0;
    unsigned int file = 0;

    ds_string_slice position = {0};
    ds_string_slice_tokenize(&fen, ' ', &position);

    for (unsigned int i = 0; i < position.len; i++) {
        char c = position.str[i];
        if (c >= '1' && c <= '8') {
            file += c - '0';
        } else if (c == '/') {
            rank++;
            file = 0;
        } else {
            char piece = 0;
            switch (c) {
                case 'P': piece = CHESS_PAWN | CHESS_WHITE; break;
                case 'R': piece = CHESS_ROOK | CHESS_WHITE; break;
                case 'N': piece = CHESS_KNIGHT | CHESS_WHITE; break;
                case 'B': piece = CHESS_BISHOP | CHESS_WHITE; break;
                case 'Q': piece = CHESS_QUEEN | CHESS_WHITE; break;
                case 'K': piece = CHESS_KING | CHESS_WHITE; break;
                case 'p': piece = CHESS_PAWN | CHESS_BLACK; break;
                case 'r': piece = CHESS_ROOK | CHESS_BLACK; break;
                case 'n': piece = CHESS_KNIGHT | CHESS_BLACK; break;
                case 'b': piece = CHESS_BISHOP | CHESS_BLACK; break;
                case 'q': piece = CHESS_QUEEN | CHESS_BLACK; break;
                case 'k': piece = CHESS_KING | CHESS_BLACK; break;
            }
            chess_square_set(board, (square_t){.rank = (CHESS_WIDTH - rank - 1), .file = file}, piece);
            file++;
        }
    }
}

char chess_square_get(chess_board_t *board, square_t square) {
    if (square.file >= 0 && square.file < CHESS_WIDTH && square.rank >= 0 && square.rank < CHESS_HEIGHT) {
        return (*board)[square.rank * CHESS_WIDTH + square.file];
    }

    return CHESS_NONE;
}

void chess_square_set(chess_board_t *board, square_t square, char piece) {
    if (square.file >= 0 && square.file < CHESS_WIDTH && square.rank >= 0 && square.rank < CHESS_HEIGHT) {
        (*board)[square.rank * CHESS_WIDTH + square.file] = piece;
    }
}

static void chess_valid_moves_pawn(chess_board_t *board, square_t square, char piece_color, ds_dynamic_array *moves) {
    char piece = 0;
    int forward_direction = (piece_color == CHESS_WHITE) ? 1 : -1;
    int second_rank = (piece_color == CHESS_WHITE) ? 1 : 6;

    square_t forward = { .file = square.file, .rank = square.rank + forward_direction };
    if (chess_square_get(board, forward) == CHESS_NONE) {
        ds_dynamic_array_append(moves, &forward);
    }

    square_t forward_left = { .file = square.file - 1, .rank = square.rank + forward_direction };
    piece = chess_square_get(board, forward_left);
    if (forward_left.file >= 0 && (piece & COLOR_FLAG) != piece_color && (piece & PIECE_FLAG) != CHESS_NONE) {
        ds_dynamic_array_append(moves, &forward_left);
    }

    square_t forward_right = { .file = square.file + 1, .rank = square.rank + forward_direction };
    piece = chess_square_get(board, forward_right);
    if (forward_right.file < CHESS_WIDTH && (piece & COLOR_FLAG) != piece_color && (piece & PIECE_FLAG) != CHESS_NONE) {
        ds_dynamic_array_append(moves, &forward_right);
    }

    square_t forward2 = { .file = square.file, .rank = square.rank + 2 * forward_direction };
    if (chess_square_get(board, forward) == CHESS_NONE && chess_square_get(board, forward2) == CHESS_NONE && square.rank == second_rank) {
        ds_dynamic_array_append(moves, &forward2);
    }
}

static void chess_valid_moves_knight(chess_board_t *board, square_t square, char piece_color, ds_dynamic_array *moves) {
    int rank_diffs[] = {2, 2, 1, -1, -2, -2, -1, 1};
    int file_diffs[] = {-1, 1, 2, 2, 1, -1, -2, -2};

    for (unsigned int i = 0; i < 8; i++) {
        int rank_diff = rank_diffs[i];
        int file_diff = file_diffs[i];

        square_t target = { .file = square.file + file_diff, .rank = square.rank + rank_diff };
        int is_bounded = (square.file >= 0 && square.file < CHESS_WIDTH && square.rank >= 0 && square.rank < CHESS_HEIGHT);
        int is_free = chess_square_get(board, target) == CHESS_NONE;
        int is_capture = (chess_square_get(board, target) & COLOR_FLAG) != piece_color;

        if (is_bounded && (is_free || is_capture)) {
            DS_UNREACHABLE(ds_dynamic_array_append(moves, &target));
        }
    }
}

static void chess_valid_moves_bishop(chess_board_t *board, square_t square, char piece_color, ds_dynamic_array *moves) {
    int rank_diffs[] = {1, 1, -1, -1};
    int file_diffs[] = {1, -1, 1, -1};

    for (unsigned int i = 0; i < 4; i++) {
        int rank_diff = rank_diffs[i];
        int file_diff = file_diffs[i];

        for (unsigned int j = 1; j < CHESS_WIDTH; j++) {
            square_t target = { .file = square.file + j * file_diff, .rank = square.rank + j * rank_diff };
            if (target.file < 0 || target.file >= CHESS_WIDTH || target.rank < 0 || target.rank >= CHESS_HEIGHT) {
                break;
            }

            char piece = chess_square_get(board, target);
            if (piece == CHESS_NONE) {
                ds_dynamic_array_append(moves, &target);
            } else if ((piece & COLOR_FLAG) != piece_color) {
                ds_dynamic_array_append(moves, &target);
                break;
            } else {
                break;
            }
        }
    }
}

static void chess_valid_moves_rook(chess_board_t *board, square_t square, char piece_color, ds_dynamic_array *moves) {
    int rank_diffs[] = {1, -1, 0, 0};
    int file_diffs[] = {0, 0, 1, -1};

    for (unsigned int i = 0; i < 4; i++) {
        int rank_diff = rank_diffs[i];
        int file_diff = file_diffs[i];

        for (unsigned int j = 1; j < CHESS_WIDTH; j++) {
            square_t target = { .file = square.file + j * file_diff, .rank = square.rank + j * rank_diff };
            if (target.file < 0 || target.file >= CHESS_WIDTH || target.rank < 0 || target.rank >= CHESS_HEIGHT) {
                break;
            }

            char piece = chess_square_get(board, target);
            if (piece == CHESS_NONE) {
                ds_dynamic_array_append(moves, &target);
            } else if ((piece & COLOR_FLAG) != piece_color) {
                ds_dynamic_array_append(moves, &target);
                break;
            } else {
                break;
            }
        }
    }
}

static void chess_valid_moves_queen(chess_board_t *board, square_t square, char piece_color, ds_dynamic_array *moves) {
    int rank_diffs[] = {1, 1, -1, -1, 1, -1, 0, 0};
    int file_diffs[] = {1, -1, 1, -1, 0, 0, 1, -1};

    for (unsigned int i = 0; i < 8; i++) {
        int rank_diff = rank_diffs[i];
        int file_diff = file_diffs[i];

        for (unsigned int j = 1; j < CHESS_WIDTH; j++) {
            square_t target = { .file = square.file + j * file_diff, .rank = square.rank + j * rank_diff };
            if (target.file < 0 || target.file >= CHESS_WIDTH || target.rank < 0 || target.rank >= CHESS_HEIGHT) {
                break;
            }

            char piece = chess_square_get(board, target);
            if (piece == CHESS_NONE) {
                ds_dynamic_array_append(moves, &target);
            } else if ((piece & COLOR_FLAG) != piece_color) {
                ds_dynamic_array_append(moves, &target);
                break;
            } else {
                break;
            }
        }
    }
}

static void chess_valid_moves_king(chess_board_t *board, square_t square, char piece_color, ds_dynamic_array *moves) {
    int rank_diffs[] = {1, 1, -1, -1, 1, -1, 0, 0};
    int file_diffs[] = {1, -1, 1, -1, 0, 0, 1, -1};

    for (unsigned int i = 0; i < 8; i++) {
        int rank_diff = rank_diffs[i];
        int file_diff = file_diffs[i];

        square_t target = { .file = square.file + file_diff, .rank = square.rank + rank_diff };
        if (target.file < 0 || target.file >= CHESS_WIDTH || target.rank < 0 || target.rank >= CHESS_HEIGHT) {
            continue;
        }

        char piece = chess_square_get(board, target);
        if (piece == CHESS_NONE) {
            ds_dynamic_array_append(moves, &target);
        } else if ((piece & COLOR_FLAG) != piece_color) {
            ds_dynamic_array_append(moves, &target);
        }
    }
}

void chess_valid_moves(chess_board_t *board, square_t square, ds_dynamic_array *moves) {
    ds_dynamic_array_clear(moves);
    char piece = chess_square_get(board, square);
    char piece_type = piece & PIECE_FLAG;
    char piece_color = piece & COLOR_FLAG;

    switch (piece_type) {
        case CHESS_PAWN:
            return chess_valid_moves_pawn(board, square, piece_color, moves);
        case CHESS_KNIGHT:
            return chess_valid_moves_knight(board, square, piece_color, moves);
        case CHESS_BISHOP:
            return chess_valid_moves_bishop(board, square, piece_color, moves);
        case CHESS_ROOK:
            return chess_valid_moves_rook(board, square, piece_color, moves);
        case CHESS_QUEEN:
            return chess_valid_moves_queen(board, square, piece_color, moves);
        case CHESS_KING:
            return chess_valid_moves_king(board, square, piece_color, moves);
        default:
            return;
    }
}

void chess_apply_move(chess_board_t *board, square_t square, square_t selected_square, ds_dynamic_array moves) {
    for (unsigned int i = 0; i < moves.count; i++) {
        square_t *move = NULL;
        DS_UNREACHABLE(ds_dynamic_array_get_ref(&moves, i, (void **)&move));
        if (move->rank == square.rank && move->file == square.file) {
            chess_square_set(board, square, chess_square_get(board, selected_square));
            chess_square_set(board, selected_square, CHESS_NONE);

            return;
        }
    }
}
