#include "chess.h"
#include "ds.h"

extern DS_ALLOCATOR allocator;

static void chess_valid_moves(const chess_state_t *state, square_t start, ds_dynamic_array *moves /* move_t */, boolean validate);

static boolean chess_can_apply_move(const chess_state_t *state, move_t move) {
    chess_state_t clone = {0};
    DS_MEMCPY(&clone, state, sizeof(chess_state_t));

    chess_apply_move(&clone, move);
    if (chess_is_in_check(&clone, clone.current_player)) {
        return false;
    }

    return true;
}

static boolean chess_validate_move(const chess_state_t *state, move_t move, boolean validate) {
    return (validate == true && chess_can_apply_move(state, move) == true) || (validate == false);
}

static void chess_valid_moves_pawn(const chess_state_t *state, square_t start, char piece_color, ds_dynamic_array *moves /* move_t */, boolean validate) {
    char piece = 0;
    int forward_direction = (piece_color == CHESS_WHITE) ? 1 : -1;
    int second_rank = (piece_color == CHESS_WHITE) ? 1 : 6;
    int promotion_rank = (piece_color == CHESS_WHITE) ? 7 : 0;

    square_t forward = { .file = start.file, .rank = start.rank + forward_direction };
    if (chess_square_get(&state->board, forward) == CHESS_NONE && chess_validate_move(state, MK_MOVE(start, forward, CHESS_MOVE, CHESS_NONE), validate)) {
        char move = CHESS_MOVE;
        if (forward.rank == promotion_rank) {
            move |= CHESS_PROMOTE;
            for (int i = 0; i < 4; i++) {
                char promote = CHESS_PROMOTE_OPTIONS[i] | piece_color;
                DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, forward, move, promote)));
            }
        } else {
            DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, forward, move, CHESS_NONE)));
        }
    }

    square_t forward_left = { .file = start.file - 1, .rank = start.rank + forward_direction };
    piece = chess_square_get(&state->board, forward_left);
    if (forward_left.file >= 0 && (piece & COLOR_FLAG) != piece_color && (piece & PIECE_FLAG) != CHESS_NONE && chess_validate_move(state, MK_MOVE(start, forward_left, CHESS_MOVE | CHESS_CAPTURE, CHESS_NONE), validate)) {
        char move = CHESS_MOVE | CHESS_CAPTURE;
        if (forward_left.rank == promotion_rank) {
            move |= CHESS_PROMOTE;
            for (int i = 0; i < 4; i++) {
                char promote = CHESS_PROMOTE_OPTIONS[i] | piece_color;
                DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, forward_left, move, promote)));
            }
        } else {
            DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, forward_left, move, CHESS_NONE)));
        }
    }

    square_t forward_right = { .file = start.file + 1, .rank = start.rank + forward_direction };
    piece = chess_square_get(&state->board, forward_right);
    if (forward_right.file < CHESS_WIDTH && (piece & COLOR_FLAG) != piece_color && (piece & PIECE_FLAG) != CHESS_NONE && chess_validate_move(state, MK_MOVE(start, forward_right, CHESS_MOVE | CHESS_CAPTURE, CHESS_NONE), validate)) {
        char move = CHESS_MOVE | CHESS_CAPTURE;
        if (forward_right.rank == promotion_rank) {
            move |= CHESS_PROMOTE;
            for (int i = 0; i < 4; i++) {
                char promote = CHESS_PROMOTE_OPTIONS[i] | piece_color;
                DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, forward_right, move, promote)));
            }
        } else {
            DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, forward_right, move, CHESS_NONE)));
        }
    }

    square_t forward2 = { .file = start.file, .rank = start.rank + 2 * forward_direction };
    if (chess_square_get(&state->board, forward) == CHESS_NONE && chess_square_get(&state->board, forward2) == CHESS_NONE && start.rank == second_rank && chess_validate_move(state, MK_MOVE(start, forward2, CHESS_MOVE, CHESS_NONE), validate)) {
        DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, forward2, CHESS_MOVE, CHESS_NONE)));
    }

    if (state->last_move) {
        char piece = chess_square_get(&state->board, state->last_move_end);
        int is_pawn = (piece & PIECE_FLAG) == CHESS_PAWN;
        int has_pushed2 = DS_ABS(state->last_move_end.rank - state->last_move_start.rank) == 2;
        int diff = state->last_move_end.file - start.file;
        int is_neighbor = DS_ABS(diff) == 1;
        int same_rank = start.rank == state->last_move_end.rank;
        int can_enp = is_pawn && has_pushed2 && is_neighbor && same_rank;

        square_t forward_enp = { .file = start.file + diff, .rank = start.rank + forward_direction };
        if (can_enp && chess_validate_move(state, MK_MOVE(start, forward_enp, CHESS_MOVE | CHESS_ENPASSANT | CHESS_CAPTURE, CHESS_NONE), validate)) {
            DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, forward_enp, CHESS_MOVE | CHESS_ENPASSANT | CHESS_CAPTURE, CHESS_NONE)));
        }
    }
}

static void chess_valid_moves_knight(const chess_state_t *state, square_t start, char piece_color, ds_dynamic_array *moves /* move_t */, boolean validate) {
    int rank_diffs[] = {2, 2, 1, -1, -2, -2, -1, 1};
    int file_diffs[] = {-1, 1, 2, 2, 1, -1, -2, -2};

    for (unsigned int i = 0; i < 8; i++) {
        int rank_diff = rank_diffs[i];
        int file_diff = file_diffs[i];

        square_t target = { .file = start.file + file_diff, .rank = start.rank + rank_diff };
        int is_bounded = (target.file >= 0 && target.file < CHESS_WIDTH && target.rank >= 0 && target.rank < CHESS_HEIGHT);
        int is_free = chess_square_get(&state->board, target) == CHESS_NONE;
        int is_capture = (chess_square_get(&state->board, target) & COLOR_FLAG) != piece_color;

        if (is_bounded && is_free) {
            if (chess_validate_move(state, MK_MOVE(start, target, CHESS_MOVE, CHESS_NONE), validate)) {
                DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, target, CHESS_MOVE, CHESS_NONE)));
            }
        } else if (is_bounded && is_capture) {
            if (chess_validate_move(state, MK_MOVE(start, target, CHESS_MOVE | CHESS_CAPTURE, CHESS_NONE), validate)) {
                DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, target, CHESS_MOVE | CHESS_CAPTURE, CHESS_NONE)));
            }
        }
    }
}

static void chess_valid_moves_bishop(const chess_state_t *state, square_t start, char piece_color, ds_dynamic_array *moves /* move_t */, boolean validate) {
    int rank_diffs[] = {1, 1, -1, -1};
    int file_diffs[] = {1, -1, 1, -1};

    for (unsigned int i = 0; i < 4; i++) {
        int rank_diff = rank_diffs[i];
        int file_diff = file_diffs[i];

        for (unsigned int j = 1; j < CHESS_WIDTH; j++) {
            square_t target = { .file = start.file + j * file_diff, .rank = start.rank + j * rank_diff };
            if (target.file < 0 || target.file >= CHESS_WIDTH || target.rank < 0 || target.rank >= CHESS_HEIGHT) {
                break;
            }

            char piece = chess_square_get(&state->board, target);
            if (piece == CHESS_NONE) {
                if (chess_validate_move(state, MK_MOVE(start, target, CHESS_MOVE, CHESS_NONE), validate)) {
                    DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, target, CHESS_MOVE, CHESS_NONE)));
                }
            } else if ((piece & COLOR_FLAG) != piece_color) {
                if (chess_validate_move(state, MK_MOVE(start, target, CHESS_MOVE | CHESS_CAPTURE, CHESS_NONE), validate)) {
                    DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, target, CHESS_MOVE | CHESS_CAPTURE, CHESS_NONE)));
                }
                break;
            } else {
                break;
            }
        }
    }
}

static void chess_valid_moves_rook(const chess_state_t *state, square_t start, char piece_color, ds_dynamic_array *moves /* move_t */, boolean validate) {
    int rank_diffs[] = {1, -1, 0, 0};
    int file_diffs[] = {0, 0, 1, -1};

    for (unsigned int i = 0; i < 4; i++) {
        int rank_diff = rank_diffs[i];
        int file_diff = file_diffs[i];

        for (unsigned int j = 1; j < CHESS_WIDTH; j++) {
            square_t target = { .file = start.file + j * file_diff, .rank = start.rank + j * rank_diff };
            if (target.file < 0 || target.file >= CHESS_WIDTH || target.rank < 0 || target.rank >= CHESS_HEIGHT) {
                break;
            }

            char piece = chess_square_get(&state->board, target);
            if (piece == CHESS_NONE) {
                if (chess_validate_move(state, MK_MOVE(start, target, CHESS_MOVE, CHESS_NONE), validate)) {
                    DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, target, CHESS_MOVE, CHESS_NONE)));
                }
            } else if ((piece & COLOR_FLAG) != piece_color) {
                if (chess_validate_move(state, MK_MOVE(start, target, CHESS_MOVE | CHESS_CAPTURE, CHESS_NONE), validate)) {
                    DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, target, CHESS_MOVE | CHESS_CAPTURE, CHESS_NONE)));
                }
                break;
            } else {
                break;
            }
        }
    }
}

static void chess_valid_moves_queen(const chess_state_t *state, square_t start, char piece_color, ds_dynamic_array *moves /* move_t */, boolean validate) {
    int rank_diffs[] = {1, 1, -1, -1, 1, -1, 0, 0};
    int file_diffs[] = {1, -1, 1, -1, 0, 0, 1, -1};

    for (unsigned int i = 0; i < 8; i++) {
        int rank_diff = rank_diffs[i];
        int file_diff = file_diffs[i];

        for (unsigned int j = 1; j < CHESS_WIDTH; j++) {
            square_t target = { .file = start.file + j * file_diff, .rank = start.rank + j * rank_diff };
            if (target.file < 0 || target.file >= CHESS_WIDTH || target.rank < 0 || target.rank >= CHESS_HEIGHT) {
                break;
            }

            char piece = chess_square_get(&state->board, target);
            if (piece == CHESS_NONE) {
                if (chess_validate_move(state, MK_MOVE(start, target, CHESS_MOVE, CHESS_NONE), validate)) {
                    DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, target, CHESS_MOVE, CHESS_NONE)));
                }
            } else if ((piece & COLOR_FLAG) != piece_color) {
                if (chess_validate_move(state, MK_MOVE(start, target, CHESS_MOVE | CHESS_CAPTURE, CHESS_NONE), validate)) {
                    DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, target, CHESS_MOVE | CHESS_CAPTURE, CHESS_NONE)));
                }
                break;
            } else {
                break;
            }
        }
    }
}

static int chess_controls(const chess_state_t *state, square_t target, char current) {
    int result = 0;

    ds_dynamic_array moves = {0};
    ds_dynamic_array_init_allocator(&moves, sizeof(move_t), &allocator);

    for (unsigned int file = 0; file < CHESS_WIDTH; file++) {
        for (unsigned int rank = 0; rank < CHESS_HEIGHT; rank++) {
            square_t square = (square_t){.file = file, .rank = rank};
            char piece = chess_square_get(&state->board, square);

            if ((piece & COLOR_FLAG) == current) {
                ds_dynamic_array_clear(&moves);
                chess_valid_moves(state, square, &moves, false);

                for (unsigned int i = 0; i < moves.count; i++) {
                    move_t *move = NULL;
                    ds_dynamic_array_get_ref(&moves, i, (void **)&move);
                    if (move->end.file == target.file && move->end.rank == target.rank) {
                        return_defer(1);
                    }
                }
            }
        }
    }

defer:
    ds_dynamic_array_free(&moves);
    return result;
}

static void chess_valid_moves_king(const chess_state_t *state, square_t start, char piece_color, ds_dynamic_array *moves /* move_t */, boolean validate) {
    int rank_diffs[] = {1, 1, -1, -1, 1, -1, 0, 0};
    int file_diffs[] = {1, -1, 1, -1, 0, 0, 1, -1};

    for (unsigned int i = 0; i < 8; i++) {
        int rank_diff = rank_diffs[i];
        int file_diff = file_diffs[i];

        square_t target = { .file = start.file + file_diff, .rank = start.rank + rank_diff };
        if (target.file < 0 || target.file >= CHESS_WIDTH || target.rank < 0 || target.rank >= CHESS_HEIGHT) {
            continue;
        }

        char piece = chess_square_get(&state->board, target);
        if (chess_validate_move(state, MK_MOVE(start, target, CHESS_MOVE, CHESS_NONE), validate)) {
            if (piece == CHESS_NONE) {
                DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, target, CHESS_MOVE, CHESS_NONE)));
            } else if ((piece & COLOR_FLAG) != piece_color) {
                DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, target, CHESS_MOVE | CHESS_CAPTURE, CHESS_NONE)));
            }
        }
    }

    int home_rank = (piece_color == CHESS_WHITE) ? 0 : 7;
    square_t king_square = MK_SQUARE(home_rank, 4);
    int is_king_home = chess_square_get(&state->board, king_square) == (CHESS_KING | piece_color);

    int is_in_check = validate == true && chess_controls(state, king_square, chess_flip_player(piece_color));
    int is_short_check = validate == true && chess_controls(state, MK_SQUARE(home_rank, 5), chess_flip_player(piece_color));
    int is_long_check = validate == true && chess_controls(state, MK_SQUARE(home_rank, 3), chess_flip_player(piece_color));

    square_t rook_short_square = MK_SQUARE(home_rank, 7);
    int is_rook_short_home = chess_square_get(&state->board, rook_short_square) == (CHESS_ROOK | piece_color);
    int is_short_free =
        chess_square_get(&state->board, MK_SQUARE(home_rank, 5)) == CHESS_NONE &&
        chess_square_get(&state->board, MK_SQUARE(home_rank, 6)) == CHESS_NONE;
    int not_moved = (state->king_moved & piece_color) == 0 && (state->short_rook_moved & piece_color) == 0;
    if (is_short_free && is_king_home && is_rook_short_home && not_moved && !is_in_check && !is_short_check) {
        if (chess_validate_move(state, MK_MOVE(start, MK_SQUARE(home_rank, 6), CHESS_MOVE | CHESS_CASTLE_SHORT, CHESS_NONE), validate)) {
            DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, MK_SQUARE(home_rank, 6), CHESS_MOVE | CHESS_CASTLE_SHORT, CHESS_NONE)));
        }
    }

    square_t rook_long_square = MK_SQUARE(home_rank, 0);
    int is_rook_long_home = chess_square_get(&state->board, rook_long_square) == (CHESS_ROOK | piece_color);
    int is_long_free =
        chess_square_get(&state->board, MK_SQUARE(home_rank, 1)) == CHESS_NONE &&
        chess_square_get(&state->board, MK_SQUARE(home_rank, 2)) == CHESS_NONE &&
        chess_square_get(&state->board, MK_SQUARE(home_rank, 3)) == CHESS_NONE;

    int can_long = (state->king_moved & piece_color) == 0 && (state->long_rook_moved & piece_color) == 0;
    if (is_long_free && is_king_home && is_rook_long_home && can_long && !is_in_check && !is_long_check) {
        if (chess_validate_move(state, MK_MOVE(start, MK_SQUARE(home_rank, 2), CHESS_MOVE | CHESS_CASTLE_LONG, CHESS_NONE), validate)) {
            DS_UNREACHABLE(ds_dynamic_array_append(moves, &MK_MOVE(start, MK_SQUARE(home_rank, 2), CHESS_MOVE | CHESS_CASTLE_LONG, CHESS_NONE)));
        }
    }
}

static void chess_valid_moves(const chess_state_t *state, square_t start, ds_dynamic_array *moves /* move_t */, boolean validate) {
    char piece = chess_square_get(&state->board, start);
    char piece_type = piece & PIECE_FLAG;
    char piece_color = piece & COLOR_FLAG;

    switch (piece_type) {
        case CHESS_PAWN:
            chess_valid_moves_pawn(state, start, piece_color, moves, validate);
            break;
        case CHESS_KNIGHT:
            chess_valid_moves_knight(state, start, piece_color, moves, validate);
            break;
        case CHESS_BISHOP:
            chess_valid_moves_bishop(state, start, piece_color, moves, validate);
            break;
        case CHESS_ROOK:
            chess_valid_moves_rook(state, start, piece_color, moves, validate);
            break;
        case CHESS_QUEEN:
            chess_valid_moves_queen(state, start, piece_color, moves, validate);
            break;
        case CHESS_KING:
            chess_valid_moves_king(state, start, piece_color, moves, validate);
            break;
        default:
            return;
    }
}

unsigned long chess_state_size(void) {
    return sizeof(chess_state_t);
}

unsigned long chess_move_size(void) {
    return sizeof(move_t);
}

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

    ds_string_slice current_player = {0};
    ds_string_slice_tokenize(&fen, ' ', &current_player);
    if (current_player.len > 0) {
        state->current_player = (current_player.str[0] == 'w') ? CHESS_WHITE : CHESS_BLACK;
    } else {
        state->current_player = CHESS_WHITE;
    }
}

char chess_square_get(const chess_board_t *board, square_t square) {
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

boolean chess_move_get(const move_t *moves, int count, move_t filter, int *index) {
    for (int i = 0; i < count; i++) {
        if (moves[i].start.file == filter.start.file && moves[i].start.rank == filter.start.rank &&
            moves[i].end.file == filter.end.file && moves[i].end.rank == filter.end.rank &&
            (moves[i].move == filter.move || filter.move == CHESS_NONE) &&
            (moves[i].promotion == filter.promotion || filter.promotion == CHESS_NONE)) {
            *index = i;
            return true;
        }
    }

    return false;
}

void chess_apply_move(chess_state_t *state, move_t move) {
    char piece = chess_square_get(&state->board, move.start);
    char piece_type = piece & PIECE_FLAG;
    char piece_color = piece & COLOR_FLAG;

    if (piece_type == CHESS_KING) {
        state->king_moved |= piece_color;
    }

    if (piece_type == CHESS_ROOK) {
        int home_rank = (piece_color == CHESS_WHITE) ? 0 : 7;
        if (move.start.file == 0 && move.start.rank == home_rank) {
            state->long_rook_moved |= piece_color;
        }

        if (move.start.file == 7 && move.start.rank == home_rank) {
            state->short_rook_moved |= piece_color;
        }
    }

    if ((move.move & CHESS_CASTLE_SHORT) != 0) {
        square_t rook = MK_SQUARE(move.start.rank, 7);
        char piece = chess_square_get(&state->board, rook);
        chess_square_set(&state->board, rook, CHESS_NONE);
        rook = MK_SQUARE(move.start.rank, 5);
        chess_square_set(&state->board, rook, piece);
    }

    if ((move.move & CHESS_CASTLE_LONG) != 0) {
        square_t rook = MK_SQUARE(move.start.rank, 0);
        char piece = chess_square_get(&state->board, rook);
        chess_square_set(&state->board, rook, CHESS_NONE);
        rook = MK_SQUARE(move.start.rank, 3);
        chess_square_set(&state->board, rook, piece);
    }

    if ((move.move & CHESS_ENPASSANT) != 0) {
        square_t take = (square_t){ .rank = move.start.rank, .file = move.end.file };
        chess_square_set(&state->board, take, CHESS_NONE);
    }

    if ((move.move & CHESS_MOVE) != 0) {
        chess_square_set(&state->board, move.end, chess_square_get(&state->board, move.start));
        chess_square_set(&state->board, move.start, CHESS_NONE);
    }

    if (move.move != CHESS_NONE) {
        state->last_move = 1;
        state->last_move_start = move.start;
        state->last_move_end = move.end;
    }

    if (move.promotion != CHESS_NONE) {
        chess_square_set(&state->board, move.end, move.promotion);
    }
}

void chess_generate_moves(const chess_state_t *state, ds_dynamic_array *moves) {
    ds_dynamic_array_clear(moves);

    for (unsigned int file = 0; file < CHESS_WIDTH; file++) {
        for (unsigned int rank = 0; rank < CHESS_HEIGHT; rank++) {
            square_t start = (square_t){.file = file, .rank = rank};
            char piece = chess_square_get(&state->board, start);

            if ((piece & COLOR_FLAG) == state->current_player) {
                chess_valid_moves(state, start, moves, true);
            }
        }
    }
}

int chess_is_in_check(const chess_state_t *state, char current) {
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
        DS_PANIC("King not found");
    }

    return chess_controls(state, king_square, enemy);
}

char chess_checkmate(const chess_state_t *state) {
    if (state->current_player == CHESS_WHITE && chess_is_checkmate(state, CHESS_WHITE)) {
        return CHESS_WHITE;
    }

    if (state->current_player == CHESS_BLACK && chess_is_checkmate(state, CHESS_BLACK)) {
        return CHESS_BLACK;
    }

    return CHESS_NONE;
}

int chess_draw(const chess_state_t *state) {
    return chess_is_stalemate(state, CHESS_WHITE) || chess_is_stalemate(state, CHESS_BLACK);
}

int chess_is_checkmate(const chess_state_t *state, char current) {
    int result = 0;

    ds_dynamic_array moves = {0};
    ds_dynamic_array_init_allocator(&moves, sizeof(move_t), &allocator);

    int check = chess_is_in_check(state, current);

    if (check == 0) {
        return_defer(0);
    }

    chess_generate_moves(state, &moves);

    if (check && moves.count == 0) {
        return_defer(1);
    }

defer:
    ds_dynamic_array_free(&moves);
    return result;
}

int chess_is_stalemate(const chess_state_t *state, char current) {
    int result = 0;

    ds_dynamic_array moves = {0};
    ds_dynamic_array_init_allocator(&moves, sizeof(move_t), &allocator);

    int check = chess_is_in_check(state, current);

    chess_generate_moves(state, &moves);

    if (check == 0 && moves.count == 0) {
        return_defer(1);
    }

defer:
    ds_dynamic_array_free(&moves);
    return result;
}

int chess_is_draw(const chess_state_t *state, char current) {
    // TODO: implement draw logic
    UNUSED(state);
    UNUSED(current);
    return 0;
}

char chess_flip_player(char current) {
    if (current == CHESS_BLACK) {
        return CHESS_WHITE;
    }

    return CHESS_BLACK;
}

int chess_count_material(const chess_state_t *state, char current) {
    int material = 0;

    for (unsigned int file = 0; file < CHESS_WIDTH; file++) {
        for (unsigned int rank = 0; rank < CHESS_HEIGHT; rank++) {
            square_t start = (square_t){.file = file, .rank = rank};
            char piece = chess_square_get(&state->board, start);

            if ((piece & COLOR_FLAG) == current) {
                if ((piece & PIECE_FLAG) == CHESS_PAWN) {
                    material += EVAL_PAWN;
                } else if ((piece & PIECE_FLAG) == CHESS_KNIGHT) {
                    material += EVAL_KNIGHT;
                } else if ((piece & PIECE_FLAG) == CHESS_BISHOP) {
                    material += EVAL_BISHOP;
                } else if ((piece & PIECE_FLAG) == CHESS_ROOK) {
                    material += EVAL_ROOK;
                } else if ((piece & PIECE_FLAG) == CHESS_QUEEN) {
                    material += EVAL_QUEEN;
                } else if ((piece & PIECE_FLAG) == CHESS_KING) {
                    material += EVAL_KING;
                }
            }
        }
    }

    return material;
}

// Where the pawn wants to go from white perspective
static chess_board_t chess_pawn_heatmap = {
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 2, 2, 2, 1, 1, 1,
    1, 2, 2, 3, 3, 2, 2, 1,
    1, 2, 3, 3, 3, 2, 2, 1,
    1, 2, 2, 3, 3, 2, 2, 1,
    2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1,
};

// Where the knight wants to go from white perspective
static chess_board_t chess_knight_heatmap = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 2, 2, 2, 1, 1, 0,
    0, 1, 2, 2, 2, 1, 1, 0,
    0, 1, 2, 2, 2, 1, 1, 0,
    0, 1, 2, 2, 2, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
};

// Where the bishop wants to go from white perspective
static chess_board_t chess_bishop_heatmap = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 2, 2, 2, 2, 1, 0,
    0, 1, 2, 3, 3, 2, 1, 0,
    0, 1, 2, 3, 3, 2, 1, 0,
    0, 1, 2, 2, 2, 2, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
};

// Where the rook wants to go from white perspective
static chess_board_t chess_rook_heatmap = {
    0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2, 2, 2, 2, 2,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 2, 2, 1, 1, 0,
    0, 1, 2, 3, 3, 2, 1, 0,
};

// Where the king wants to go from white perspective
static chess_board_t chess_king_heatmap = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 2, 1, 1, 1, 1, 2, 0,
};

int chess_count_material_weighted(const chess_state_t *state, char current) {
    int material = 0;

    for (unsigned int file = 0; file < CHESS_WIDTH; file++) {
        for (unsigned int rank = 0; rank < CHESS_HEIGHT; rank++) {
            square_t start = (square_t){.file = file, .rank = rank};
            square_t heat = (current == CHESS_WHITE) ? (square_t){.file = file, .rank = rank} : (square_t){.file = file, .rank = CHESS_HEIGHT - rank - 1};
            char piece = chess_square_get(&state->board, start);

            if ((piece & COLOR_FLAG) == current) {
                if ((piece & PIECE_FLAG) == CHESS_PAWN) {
                    material += (EVAL_PAWN + 10 * chess_square_get(&chess_pawn_heatmap, heat));
                } else if ((piece & PIECE_FLAG) == CHESS_KNIGHT) {
                    material += (EVAL_KNIGHT + 10 * chess_square_get(&chess_knight_heatmap, heat));
                } else if ((piece & PIECE_FLAG) == CHESS_BISHOP) {
                    material += (EVAL_BISHOP + 10 * chess_square_get(&chess_bishop_heatmap, heat));
                } else if ((piece & PIECE_FLAG) == CHESS_ROOK) {
                    material += (EVAL_ROOK + 10 * chess_square_get(&chess_rook_heatmap, heat));
                } else if ((piece & PIECE_FLAG) == CHESS_QUEEN) {
                    material += EVAL_QUEEN;
                } else if ((piece & PIECE_FLAG) == CHESS_KING) {
                    material += (EVAL_KING + 10 * chess_square_get(&chess_king_heatmap, heat));
                }
            }
        }
    }

    return material;
}

void chess_count_positions(const chess_state_t *state, int depth, perft_t *perft) {
    if (chess_is_in_check(state, state->current_player)) {
        perft->checks += 1;
    }

    if (chess_is_checkmate(state, state->current_player)) {
        if (depth == 0) {
            perft->nodes += 1;
        }
        perft->checkmates += 1;
        return;
    }

    if (depth == 0) {
        perft->nodes += 1;
        return;
    }

    ds_dynamic_array moves = {0};
    ds_dynamic_array_init_allocator(&moves, sizeof(move_t), &allocator);

    for (unsigned int file = 0; file < CHESS_WIDTH; file++) {
        for (unsigned int rank = 0; rank < CHESS_HEIGHT; rank++) {
            square_t start = (square_t){.file = file, .rank = rank};
            char piece = chess_square_get(&state->board, start);

            if ((piece & COLOR_FLAG) == state->current_player) {
                ds_dynamic_array_clear(&moves);
                chess_valid_moves(state, start, &moves, true);

                for (unsigned int i = 0; i < moves.count; i++) {
                    move_t *move = NULL;
                    DS_UNREACHABLE(ds_dynamic_array_get_ref(&moves, i, (void **)&move));

                    if ((move->move & CHESS_ENPASSANT) != 0) {
                        perft->enp += 1;
                    }

                    if ((move->move & CHESS_CASTLE_SHORT) != 0 || (move->move & CHESS_CASTLE_LONG) != 0) {
                        perft->castles += 1;
                    }

                    if ((move->move & CHESS_PROMOTE) != 0) {
                        perft->promote += 1;
                    }

                    if ((move->move & CHESS_CAPTURE) != 0) {
                        perft->captures += 1;
                    }

                    if (move->move != CHESS_NONE) {
                        chess_state_t clone = {0};
                        DS_MEMCPY(&clone, state, sizeof(chess_state_t));

                        chess_apply_move(&clone, *move);

                        clone.current_player = chess_flip_player(clone.current_player);

                        chess_count_positions(&clone, depth - 1, perft);
                    }
                }
            }
        }
    }

    ds_dynamic_array_free(&moves);
}
