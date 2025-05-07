#include "chess.h"

void chess_init_fen(chess_state_t *state, ds_string_slice fen) {
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
            chess_square_set(&state->board, (square_t){.rank = (CHESS_WIDTH - rank - 1), .file = file}, piece);
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

void chess_apply_move(chess_state_t *state, square_t start, square_t end, char move) {
    char piece = chess_square_get(&state->board, start);
    char piece_type = piece & PIECE_FLAG;
    char piece_color = piece & COLOR_FLAG;

    if (piece_type == CHESS_KING) {
        state->king_moved |= piece_color;
    }

    if (piece_type == CHESS_ROOK) {
        int home_rank = (piece_color == CHESS_WHITE) ? 0 : 7;
        if (start.file == 0 && start.rank == home_rank) {
            state->long_rook_moved |= piece_color;
        }

        if (start.file == 7 && start.rank == home_rank) {
            state->short_rook_moved |= piece_color;
        }
    }

    if ((move & CHESS_CASTLE_SHORT) != 0) {
        square_t rook = MK_SQUARE(start.rank, 7);
        char piece = chess_square_get(&state->board, rook);
        chess_square_set(&state->board, rook, CHESS_NONE);
        rook = MK_SQUARE(start.rank, 5);
        chess_square_set(&state->board, rook, piece);
    }

    if ((move & CHESS_CASTLE_LONG) != 0) {
        square_t rook = MK_SQUARE(start.rank, 0);
        char piece = chess_square_get(&state->board, rook);
        chess_square_set(&state->board, rook, CHESS_NONE);
        rook = MK_SQUARE(start.rank, 3);
        chess_square_set(&state->board, rook, piece);
    }

    if ((move & CHESS_ENPASSANT) != 0) {
        square_t take = (square_t){ .rank = start.rank, .file = end.file };
        chess_square_set(&state->board, take, CHESS_NONE);
    }

    if ((move & CHESS_CAPTURE) != 0) {
        chess_square_set(&state->board, end, CHESS_NONE);
    }

    if ((move & CHESS_MOVE) != 0) {
        chess_square_set(&state->board, end, chess_square_get(&state->board, start));
        chess_square_set(&state->board, start, CHESS_NONE);
    }

    if (move != CHESS_NONE) {
        state->last_move = 1;
        state->last_move_start = start;
        state->last_move_end = end;
    }
}

static void chess_valid_moves_pawn(chess_state_t *state, square_t start, char piece_color, chess_board_t *moves) {
    char piece = 0;
    int forward_direction = (piece_color == CHESS_WHITE) ? 1 : -1;
    int second_rank = (piece_color == CHESS_WHITE) ? 1 : 6;

    square_t forward = { .file = start.file, .rank = start.rank + forward_direction };
    if (chess_square_get(&state->board, forward) == CHESS_NONE) {
        chess_square_set(moves, forward, CHESS_MOVE);
    }

    square_t forward_left = { .file = start.file - 1, .rank = start.rank + forward_direction };
    piece = chess_square_get(&state->board, forward_left);
    if (forward_left.file >= 0 && (piece & COLOR_FLAG) != piece_color && (piece & PIECE_FLAG) != CHESS_NONE) {
        chess_square_set(moves, forward_left, CHESS_MOVE | CHESS_CAPTURE);
    }

    square_t forward_right = { .file = start.file + 1, .rank = start.rank + forward_direction };
    piece = chess_square_get(&state->board, forward_right);
    if (forward_right.file < CHESS_WIDTH && (piece & COLOR_FLAG) != piece_color && (piece & PIECE_FLAG) != CHESS_NONE) {
        chess_square_set(moves, forward_right, CHESS_MOVE | CHESS_CAPTURE);
    }

    square_t forward2 = { .file = start.file, .rank = start.rank + 2 * forward_direction };
    if (chess_square_get(&state->board, forward) == CHESS_NONE && chess_square_get(&state->board, forward2) == CHESS_NONE && start.rank == second_rank) {
        chess_square_set(moves, forward2, CHESS_MOVE);
    }

    if (state->last_move) {
        char piece = chess_square_get(&state->board, state->last_move_end);
        int is_pawn = (piece & PIECE_FLAG) == CHESS_PAWN;
        int has_pushed2 = DS_ABS(state->last_move_end.rank - state->last_move_start.rank) == 2;
        int diff = state->last_move_end.file - start.file;
        int is_neighbor = DS_ABS(diff) == 1;
        int same_rank = start.rank == state->last_move_end.rank;
        int can_enp = is_pawn && has_pushed2 && is_neighbor && same_rank;

        if (can_enp) {
            square_t forward_enp = { .file = start.file + diff, .rank = start.rank + forward_direction };
            chess_square_set(moves, forward_enp, CHESS_MOVE | CHESS_ENPASSANT);
        }
    }
}

static void chess_valid_moves_knight(chess_state_t *state, square_t start, char piece_color, chess_board_t *moves) {
    int rank_diffs[] = {2, 2, 1, -1, -2, -2, -1, 1};
    int file_diffs[] = {-1, 1, 2, 2, 1, -1, -2, -2};

    for (unsigned int i = 0; i < 8; i++) {
        int rank_diff = rank_diffs[i];
        int file_diff = file_diffs[i];

        square_t target = { .file = start.file + file_diff, .rank = start.rank + rank_diff };
        int is_bounded = (target.file >= 0 && target.file < CHESS_WIDTH && target.rank >= 0 && target.rank < CHESS_HEIGHT);
        int is_free = chess_square_get(&state->board, target) == CHESS_NONE;
        int is_capture = (chess_square_get(&state->board, target) & COLOR_FLAG) != piece_color;

        char move = CHESS_NONE;
        if (is_bounded && is_free) {
            move = CHESS_MOVE;
        } else if (is_bounded && is_capture) {
            move = CHESS_MOVE | CHESS_CAPTURE;
        }

        chess_square_set(moves, target, move);
    }
}

static void chess_valid_moves_bishop(chess_state_t *state, square_t square, char piece_color, chess_board_t *moves) {
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

            char piece = chess_square_get(&state->board, target);
            if (piece == CHESS_NONE) {
                chess_square_set(moves, target, CHESS_MOVE);
            } else if ((piece & COLOR_FLAG) != piece_color) {
                chess_square_set(moves, target, CHESS_MOVE | CHESS_CAPTURE);
                break;
            } else {
                break;
            }
        }
    }
}

static void chess_valid_moves_rook(chess_state_t *state, square_t square, char piece_color, chess_board_t *moves) {
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

            char piece = chess_square_get(&state->board, target);
            if (piece == CHESS_NONE) {
                chess_square_set(moves, target, CHESS_MOVE);
            } else if ((piece & COLOR_FLAG) != piece_color) {
                chess_square_set(moves, target, CHESS_MOVE | CHESS_CAPTURE);
                break;
            } else {
                break;
            }
        }
    }
}

static void chess_valid_moves_queen(chess_state_t *state, square_t square, char piece_color, chess_board_t *moves) {
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

            char piece = chess_square_get(&state->board, target);
            if (piece == CHESS_NONE) {
                chess_square_set(moves, target, CHESS_MOVE);
            } else if ((piece & COLOR_FLAG) != piece_color) {
                chess_square_set(moves, target, CHESS_MOVE | CHESS_CAPTURE);
                break;
            } else {
                break;
            }
        }
    }
}

static void chess_valid_moves_king(chess_state_t *state, square_t square, char piece_color, chess_board_t *moves) {
    int rank_diffs[] = {1, 1, -1, -1, 1, -1, 0, 0};
    int file_diffs[] = {1, -1, 1, -1, 0, 0, 1, -1};

    for (unsigned int i = 0; i < 8; i++) {
        int rank_diff = rank_diffs[i];
        int file_diff = file_diffs[i];

        square_t target = { .file = square.file + file_diff, .rank = square.rank + rank_diff };
        if (target.file < 0 || target.file >= CHESS_WIDTH || target.rank < 0 || target.rank >= CHESS_HEIGHT) {
            continue;
        }

        char piece = chess_square_get(&state->board, target);
        if (piece == CHESS_NONE) {
            chess_square_set(moves, target, CHESS_MOVE);
        } else if ((piece & COLOR_FLAG) != piece_color) {
            chess_square_set(moves, target, CHESS_MOVE | CHESS_CAPTURE);
        }
    }

    int home_rank = (piece_color == CHESS_WHITE) ? 0 : 7;
    square_t king_square = MK_SQUARE(home_rank, 4);
    int is_king_home = chess_square_get(&state->board, king_square) == (CHESS_KING | piece_color);

    square_t rook_short_square = MK_SQUARE(home_rank, 7);
    int is_rook_short_home = chess_square_get(&state->board, rook_short_square) == (CHESS_ROOK | piece_color);
    int is_short_free =
        chess_square_get(&state->board, MK_SQUARE(home_rank, 5)) == CHESS_NONE &&
        chess_square_get(&state->board, MK_SQUARE(home_rank, 6)) == CHESS_NONE;

    int can_short = (state->king_moved & piece_color) == 0 && (state->short_rook_moved & piece_color) == 0;
    if (is_short_free && is_king_home && is_rook_short_home && can_short) {
        chess_square_set(moves, MK_SQUARE(home_rank, 6), CHESS_MOVE | CHESS_CASTLE_SHORT);
    }

    square_t rook_long_square = MK_SQUARE(home_rank, 0);
    int is_rook_long_home = chess_square_get(&state->board, rook_long_square) == (CHESS_ROOK | piece_color);
    int is_long_free =
        chess_square_get(&state->board, MK_SQUARE(home_rank, 1)) == CHESS_NONE &&
        chess_square_get(&state->board, MK_SQUARE(home_rank, 2)) == CHESS_NONE &&
        chess_square_get(&state->board, MK_SQUARE(home_rank, 3)) == CHESS_NONE;

    int can_long = (state->king_moved & piece_color) == 0 && (state->long_rook_moved & piece_color) == 0;
    if (is_long_free && is_king_home && is_rook_long_home && can_long) {
        chess_square_set(moves, MK_SQUARE(home_rank, 2), CHESS_MOVE | CHESS_CASTLE_LONG);
    }
}

void chess_valid_moves_all(chess_state_t *state, square_t start, chess_board_t *moves) {
    DS_MEMSET(moves, 0, sizeof(chess_board_t));
    char piece = chess_square_get(&state->board, start);
    char piece_type = piece & PIECE_FLAG;
    char piece_color = piece & COLOR_FLAG;

    switch (piece_type) {
        case CHESS_PAWN:
            return chess_valid_moves_pawn(state, start, piece_color, moves);
        case CHESS_KNIGHT:
            return chess_valid_moves_knight(state, start, piece_color, moves);
        case CHESS_BISHOP:
            return chess_valid_moves_bishop(state, start, piece_color, moves);
        case CHESS_ROOK:
            return chess_valid_moves_rook(state, start, piece_color, moves);
        case CHESS_QUEEN:
            return chess_valid_moves_queen(state, start, piece_color, moves);
        case CHESS_KING:
            return chess_valid_moves_king(state, start, piece_color, moves);
        default:
            return;
    }
}

void chess_valid_moves(chess_state_t *state, square_t start, chess_board_t *moves) {
    char piece = chess_square_get(&state->board, start);
    char piece_type = piece & PIECE_FLAG;
    char piece_color = piece & COLOR_FLAG;

    chess_valid_moves_all(state, start, moves);

    for (unsigned int file_i = 0; file_i < CHESS_WIDTH; file_i++) {
        for (unsigned int rank_i = 0; rank_i < CHESS_HEIGHT; rank_i++) {
            square_t end = (square_t){.file = file_i, .rank = rank_i};
            char move = chess_square_get(moves, end);

            if (piece_type == CHESS_KING) {
                int home_rank = (piece_color == CHESS_WHITE) ? 0 : 7;
                square_t king_square = MK_SQUARE(home_rank, 4);

                int is_in_check = chess_controls(state, king_square, chess_flip_player(piece_color));
                int is_short_check = chess_controls(state, MK_SQUARE(home_rank, 5), chess_flip_player(piece_color));
                int is_long_check = chess_controls(state, MK_SQUARE(home_rank, 3), chess_flip_player(piece_color));

                if ((move & CHESS_CASTLE_SHORT) != 0 && (is_in_check || is_short_check)) {
                    chess_square_set(moves, end, CHESS_NONE);
                }

                if ((move & CHESS_CASTLE_LONG) != 0 && (is_in_check || is_long_check)) {
                    chess_square_set(moves, end, CHESS_NONE);
                }
            }

            if (move != CHESS_NONE) {
                chess_state_t clone = {0};
                DS_MEMCPY(&clone, state, sizeof(chess_state_t));

                chess_apply_move(&clone, start, end, move);
                if (chess_is_in_check(&clone, piece_color)) {
                    chess_square_set(moves, end, CHESS_NONE);
                }
            }
        }
    }
}

int chess_is_in_check(chess_state_t *state, char current) {
    char enemy = chess_flip_player(current);

    square_t king_square = {0};
    int found = 0;
    for (unsigned int file = 0; file < CHESS_WIDTH && found == 0; file++) {
        for (unsigned int rank = 0; rank < CHESS_HEIGHT && found == 0; rank++) {
            square_t square = (square_t){.file = file, .rank = rank};
            char piece = chess_square_get(&state->board, square);

            if (piece == (CHESS_KING | current)) {
                king_square = square;
                found = 1;
            }
        }
    }

    if (found == 0) {
        DS_PANIC("Playing without a king... smh");
    }

    return chess_controls(state, king_square, enemy);
}

int chess_controls(chess_state_t *state, square_t target, char current) {
    for (unsigned int file = 0; file < CHESS_WIDTH; file++) {
        for (unsigned int rank = 0; rank < CHESS_HEIGHT; rank++) {
            square_t square = (square_t){.file = file, .rank = rank};
            char piece = chess_square_get(&state->board, square);

            if ((piece & COLOR_FLAG) == current) {
                chess_board_t moves = {0};
                chess_valid_moves_all(state, square, &moves);

                char move = chess_square_get(&moves, target);
                if (move != CHESS_NONE) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

char chess_flip_player(char current) {
    if (current == CHESS_BLACK) {
        return CHESS_WHITE;
    }

    return CHESS_BLACK;
}

int chess_count_positions(chess_state_t *state, char current, int depth) {
    if (depth == 0) {
        return 1;
    }

    int count = 0;
    for (unsigned int file = 0; file < CHESS_WIDTH; file++) {
        for (unsigned int rank = 0; rank < CHESS_HEIGHT; rank++) {
            square_t start = (square_t){.file = file, .rank = rank};
            char piece = chess_square_get(&state->board, start);

            if ((piece & COLOR_FLAG) == current) {
                chess_board_t moves = {0};
                chess_valid_moves(state, start, &moves);

                for (unsigned int file_i = 0; file_i < CHESS_WIDTH; file_i++) {
                    for (unsigned int rank_i = 0; rank_i < CHESS_HEIGHT; rank_i++) {
                        square_t end = (square_t){.file = file_i, .rank = rank_i};
                        char move = chess_square_get(&moves, end);

                        if (move != CHESS_NONE) {
                            chess_state_t clone = {0};
                            DS_MEMCPY(&clone, state, sizeof(chess_state_t));

                            chess_apply_move(&clone, start, end, move);
                            count += chess_count_positions(&clone, chess_flip_player(current), depth - 1);
                        }
                    }
                }
            }
        }
    }

    return count;
}
