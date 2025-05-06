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

void chess_apply_move(chess_board_t *board, move_t move) {
    if (move.takes) {
        chess_square_set(board, move.takes_square, CHESS_NONE);
    }
    if (move.is_move) {
        chess_square_set(board, move.end, chess_square_get(board, move.start));
        chess_square_set(board, move.start, CHESS_NONE);
    }
}

static void chess_valid_moves_pawn(chess_board_t *board, square_t square, move_t last_move, char piece_color, ds_dynamic_array *moves) {
    char piece = 0;
    int forward_direction = (piece_color == CHESS_WHITE) ? 1 : -1;
    int second_rank = (piece_color == CHESS_WHITE) ? 1 : 6;

    square_t forward = { .file = square.file, .rank = square.rank + forward_direction };
    if (chess_square_get(board, forward) == CHESS_NONE) {
        move_t move = MK_MOVE(square, forward);
        ds_dynamic_array_append(moves, &move);
    }

    square_t forward_left = { .file = square.file - 1, .rank = square.rank + forward_direction };
    piece = chess_square_get(board, forward_left);
    if (forward_left.file >= 0 && (piece & COLOR_FLAG) != piece_color && (piece & PIECE_FLAG) != CHESS_NONE) {
        move_t move = MK_MOVE_TAKES(square, forward_left, forward_left);
        ds_dynamic_array_append(moves, &move);
    }

    square_t forward_right = { .file = square.file + 1, .rank = square.rank + forward_direction };
    piece = chess_square_get(board, forward_right);
    if (forward_right.file < CHESS_WIDTH && (piece & COLOR_FLAG) != piece_color && (piece & PIECE_FLAG) != CHESS_NONE) {
        move_t move = MK_MOVE_TAKES(square, forward_right, forward_right);
        ds_dynamic_array_append(moves, &move);
    }

    square_t forward2 = { .file = square.file, .rank = square.rank + 2 * forward_direction };
    if (chess_square_get(board, forward) == CHESS_NONE && chess_square_get(board, forward2) == CHESS_NONE && square.rank == second_rank) {
        move_t move = MK_MOVE(square, forward2);
        ds_dynamic_array_append(moves, &move);
    }

    int has_moved = !(last_move.start.file == 0 && last_move.start.rank == 0 && last_move.end.file == 0 && last_move.end.rank == 0);
    if (has_moved) {
        char piece = chess_square_get(board, last_move.end);
        int is_pawn = (piece & PIECE_FLAG) == CHESS_PAWN;
        int has_pushed2 = DS_ABS(last_move.end.rank - last_move.start.rank) == 2;
        int diff = last_move.end.file - square.file;
        int is_neighbor = DS_ABS(diff) == 1;
        int same_rank = square.rank == last_move.end.rank;
        int can_enp = is_pawn && has_pushed2 && is_neighbor && same_rank;

        if (can_enp) {
            square_t forward_enp = { .file = square.file + diff, .rank = square.rank + forward_direction };
            move_t move = MK_MOVE_TAKES(square, forward_enp, last_move.end);
            ds_dynamic_array_append(moves, &move);
        }
    }
}

static void chess_valid_moves_knight(chess_board_t *board, square_t square, char piece_color, ds_dynamic_array *moves) {
    int rank_diffs[] = {2, 2, 1, -1, -2, -2, -1, 1};
    int file_diffs[] = {-1, 1, 2, 2, 1, -1, -2, -2};

    for (unsigned int i = 0; i < 8; i++) {
        int rank_diff = rank_diffs[i];
        int file_diff = file_diffs[i];

        square_t target = { .file = square.file + file_diff, .rank = square.rank + rank_diff };
        int is_bounded = (target.file >= 0 && target.file < CHESS_WIDTH && target.rank >= 0 && target.rank < CHESS_HEIGHT);
        int is_free = chess_square_get(board, target) == CHESS_NONE;
        int is_capture = (chess_square_get(board, target) & COLOR_FLAG) != piece_color;

        if (is_bounded && is_free) {
            move_t move = MK_MOVE(square, target);
            DS_UNREACHABLE(ds_dynamic_array_append(moves, &move));
        } else if (is_bounded && is_capture) {
            move_t move = MK_MOVE_TAKES(square, target, target);
            DS_UNREACHABLE(ds_dynamic_array_append(moves, &move));
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
                move_t move = MK_MOVE(square, target);
                DS_UNREACHABLE(ds_dynamic_array_append(moves, &move));
            } else if ((piece & COLOR_FLAG) != piece_color) {
                move_t move = MK_MOVE_TAKES(square, target, target);
                DS_UNREACHABLE(ds_dynamic_array_append(moves, &move));
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
                move_t move = MK_MOVE(square, target);
                DS_UNREACHABLE(ds_dynamic_array_append(moves, &move));
            } else if ((piece & COLOR_FLAG) != piece_color) {
                move_t move = MK_MOVE_TAKES(square, target, target);
                DS_UNREACHABLE(ds_dynamic_array_append(moves, &move));
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
                move_t move = MK_MOVE(square, target);
                DS_UNREACHABLE(ds_dynamic_array_append(moves, &move));
            } else if ((piece & COLOR_FLAG) != piece_color) {
                move_t move = MK_MOVE_TAKES(square, target, target);
                DS_UNREACHABLE(ds_dynamic_array_append(moves, &move));
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
            move_t move = MK_MOVE(square, target);
            DS_UNREACHABLE(ds_dynamic_array_append(moves, &move));
        } else if ((piece & COLOR_FLAG) != piece_color) {
            move_t move = MK_MOVE_TAKES(square, target, target);
            DS_UNREACHABLE(ds_dynamic_array_append(moves, &move));
        }
    }
}

static void chess_valid_moves_all(chess_board_t *board, square_t square, move_t last_move, ds_dynamic_array *moves /* move_t */) {
    ds_dynamic_array_clear(moves);
    char piece = chess_square_get(board, square);
    char piece_type = piece & PIECE_FLAG;
    char piece_color = piece & COLOR_FLAG;

    switch (piece_type) {
        case CHESS_PAWN:
            return chess_valid_moves_pawn(board, square, last_move, piece_color, moves);
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

void chess_valid_moves(chess_board_t *board, square_t square, move_t last_move, ds_dynamic_array *moves /* move_t */) {
    char piece = chess_square_get(board, square);
    char piece_color = piece & COLOR_FLAG;

    chess_valid_moves_all(board, square, last_move, moves);

    for (int i = moves->count - 1; i >= 0; i--) {
        move_t *move = NULL;
        ds_dynamic_array_get_ref(moves, i, (void **)&move);

        chess_board_t clone = {0};
        DS_MEMCPY(&clone, board, CHESS_WIDTH * CHESS_HEIGHT * sizeof(char));

        chess_apply_move(&clone, *move);
        if (chess_is_in_check(&clone, piece_color, moves->allocator)) {
            ds_dynamic_array_delete(moves, i);
        }
    }
}

int chess_is_in_check(chess_board_t *board, char current, DS_ALLOCATOR *allocator) {
    char enemy = chess_flip_player(current);

    for (unsigned int file = 0; file < CHESS_WIDTH; file++) {
        for (unsigned int rank = 0; rank < CHESS_HEIGHT; rank++) {
            square_t square = (square_t){.file = file, .rank = rank};
            char piece = chess_square_get(board, square);

            if ((piece & COLOR_FLAG) == enemy) {
                ds_dynamic_array moves = {0};
                ds_dynamic_array_init_allocator(&moves, sizeof(move_t), allocator);
                chess_valid_moves_all(board, square, (move_t){0}, &moves);

                for (unsigned int i = 0; i < moves.count; i++) {
                    move_t *move = NULL;
                    ds_dynamic_array_get_ref(&moves, i, (void **)&move);

                    char piece = chess_square_get(board, move->end);
                    if ((piece & PIECE_FLAG) == CHESS_KING) {
                        return 1;
                    }
                }

                ds_dynamic_array_free(&moves);
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

int chess_count_positions(chess_board_t *board, move_t last_move, char current, int depth, DS_ALLOCATOR *allocator) {
    if (depth == 0) {
        return 1;
    }

    int count = 0;
    for (unsigned int file = 0; file < CHESS_WIDTH; file++) {
        for (unsigned int rank = 0; rank < CHESS_HEIGHT; rank++) {
            square_t square = (square_t){.file = file, .rank = rank};
            char piece = chess_square_get(board, square);

            if ((piece & COLOR_FLAG) == current) {
                ds_dynamic_array moves = {0};
                ds_dynamic_array_init_allocator(&moves, sizeof(move_t), allocator);
                chess_valid_moves(board, square, last_move, &moves);

                for (unsigned int i = 0; i < moves.count; i++) {
                    move_t *move = NULL;
                    ds_dynamic_array_get_ref(&moves, i, (void **)&move);

                    chess_board_t clone = {0};
                    DS_MEMCPY(&clone, board, CHESS_WIDTH * CHESS_HEIGHT * sizeof(char));

                    chess_apply_move(&clone, *move);
                    count += chess_count_positions(&clone, *move, chess_flip_player(current), depth - 1, allocator);
                }

                ds_dynamic_array_free(&moves);
            }
        }
    }

    return count;
}
