#define WASM_IMPLEMENTATION
#include "wasm.h"

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

DS_ALLOCATOR allocator;
chess_board_t board;
square_t selected_square = {0};
int is_selected = 0;
ds_dynamic_array moves = {0};

static char chess_square_get(square_t square) {
    if (square.file >= 0 && square.file < CHESS_WIDTH && square.rank >= 0 && square.rank < CHESS_HEIGHT) {
        return board[square.rank * CHESS_WIDTH + square.file];
    }

    return CHESS_NONE;
}

static void chess_square_set(square_t square, char piece) {
    if (square.file >= 0 && square.file < CHESS_WIDTH && square.rank >= 0 && square.rank < CHESS_HEIGHT) {
        board[square.rank * CHESS_WIDTH + square.file] = piece;
    }
}

static const char *chess_piece_texture_path(char piece) {
    switch (piece) {
        case CHESS_PAWN | CHESS_WHITE:
            return "Chess_plt60.png";
        case CHESS_PAWN | CHESS_BLACK:
            return "Chess_pdt60.png";
        case CHESS_ROOK | CHESS_WHITE:
            return "Chess_rlt60.png";
        case CHESS_ROOK | CHESS_BLACK:
            return "Chess_rdt60.png";
        case CHESS_KNIGHT | CHESS_WHITE:
            return "Chess_nlt60.png";
        case CHESS_KNIGHT | CHESS_BLACK:
            return "Chess_ndt60.png";
        case CHESS_BISHOP | CHESS_WHITE:
            return "Chess_blt60.png";
        case CHESS_BISHOP | CHESS_BLACK:
            return "Chess_bdt60.png";
        case CHESS_QUEEN | CHESS_WHITE:
            return "Chess_qlt60.png";
        case CHESS_QUEEN | CHESS_BLACK:
            return "Chess_qdt60.png";
        case CHESS_KING | CHESS_WHITE:
            return "Chess_klt60.png";
        case CHESS_KING | CHESS_BLACK:
            return "Chess_kdt60.png";
        default:
            return NULL;
    }
}

static void px_to_square(ivec2 *px, square_t *square) {
    int canvas_width = js_width();
    int canvas_height = js_height();

    int cell_width = canvas_width / CHESS_WIDTH;
    int cell_height = canvas_height / CHESS_HEIGHT;

    square->file = px->x / cell_width;
    square->rank = (CHESS_HEIGHT - px->y / cell_height - 1);
}

static void square_to_px(square_t *square, ivec2 *px) {
    int canvas_width = js_width();
    int canvas_height = js_height();

    int cell_width = canvas_width / CHESS_WIDTH;
    int cell_height = canvas_height / CHESS_HEIGHT;

    px->x = square->file * cell_width;
    px->y = (CHESS_WIDTH - square->rank - 1) * cell_height;
}

static void chess_load_fen(ds_string_slice fen) {
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
            chess_square_set((square_t){.rank = (CHESS_WIDTH - rank - 1), .file = file}, piece);
            file++;
        }
    }
}

static void chess_print_board() {
    int canvas_width = js_width();
    int canvas_height = js_height();

    int cell_width = canvas_width / CHESS_WIDTH;
    int cell_height = canvas_height / CHESS_HEIGHT;

    for (unsigned int file = 0; file < CHESS_WIDTH; file++) {
        for (unsigned int rank = 0; rank < CHESS_HEIGHT; rank++) {
            int is_light_square = (file + rank) % 2 == 1;
            int square_color = is_light_square ? SQUARE_LIGHT : SQUARE_DARK;

            int file_px = file * cell_width;
            int rank_px = (CHESS_WIDTH - rank - 1) * cell_height;

            js_fill_rect(file_px, rank_px, cell_width, cell_height, square_color);
            char piece = chess_square_get((square_t){.rank = rank, .file = file});
            if (piece != CHESS_NONE) {
                js_draw_texture(file_px, rank_px, cell_width, cell_height, chess_piece_texture_path(piece));
            }
        }
    }

    for (unsigned int i = 0; i < moves.count; i++) {
        square_t *square = NULL;
        DS_UNREACHABLE(ds_dynamic_array_get_ref(&moves, i, (void **)&square));

        ivec2 px = {0};
        square_to_px(square, &px);

        js_fill_circle(px.x + cell_width / 2, px.y + cell_height / 2, cell_width / 4, SQUARE_MOVE);
    }
}

static void chess_valid_moves_pawn(square_t square, char piece_color) {
    char piece = 0;
    int forward_direction = (piece_color == CHESS_WHITE) ? 1 : -1;
    int second_rank = (piece_color == CHESS_WHITE) ? 1 : 6;

    square_t forward = { .file = square.file, .rank = square.rank + forward_direction };
    if (chess_square_get(forward) == CHESS_NONE) {
        ds_dynamic_array_append(&moves, &forward);
    }

    square_t forward_left = { .file = square.file - 1, .rank = square.rank + forward_direction };
    piece = chess_square_get(forward_left);
    if (forward_left.file >= 0 && (piece & COLOR_FLAG) != piece_color && (piece & PIECE_FLAG) != CHESS_NONE) {
        ds_dynamic_array_append(&moves, &forward_left);
    }

    square_t forward_right = { .file = square.file + 1, .rank = square.rank + forward_direction };
    piece = chess_square_get(forward_right);
    if (forward_right.file < CHESS_WIDTH && (piece & COLOR_FLAG) != piece_color && (piece & PIECE_FLAG) != CHESS_NONE) {
        ds_dynamic_array_append(&moves, &forward_right);
    }

    square_t forward2 = { .file = square.file, .rank = square.rank + 2 * forward_direction };
    if (chess_square_get(forward) == CHESS_NONE && chess_square_get(forward2) == CHESS_NONE && square.rank == second_rank) {
        ds_dynamic_array_append(&moves, &forward2);
    }
}

static void chess_valid_moves_knight(square_t square, char piece_color) {
    int rank_diffs[] = {2, 2, 1, -1, -2, -2, -1, 1};
    int file_diffs[] = {-1, 1, 2, 2, 1, -1, -2, -2};

    for (unsigned int i = 0; i < 8; i++) {
        int rank_diff = rank_diffs[i];
        int file_diff = file_diffs[i];

        square_t target = { .file = square.file + file_diff, .rank = square.rank + rank_diff };
        int is_bounded = (square.file >= 0 && square.file < CHESS_WIDTH && square.rank >= 0 && square.rank < CHESS_HEIGHT);
        int is_free = chess_square_get(target) == CHESS_NONE;
        int is_capture = (chess_square_get(target) & COLOR_FLAG) != piece_color;

        if (is_bounded && (is_free || is_capture)) {
            DS_UNREACHABLE(ds_dynamic_array_append(&moves, &target));
        }
    }
}

static void chess_valid_moves_bishop(square_t square, char piece_color) {
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

            char piece = chess_square_get(target);
            if (piece == CHESS_NONE) {
                ds_dynamic_array_append(&moves, &target);
            } else if ((piece & COLOR_FLAG) != piece_color) {
                ds_dynamic_array_append(&moves, &target);
                break;
            } else {
                break;
            }
        }
    }
}

static void chess_valid_moves_rook(square_t square, char piece_color) {
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

            char piece = chess_square_get(target);
            if (piece == CHESS_NONE) {
                ds_dynamic_array_append(&moves, &target);
            } else if ((piece & COLOR_FLAG) != piece_color) {
                ds_dynamic_array_append(&moves, &target);
                break;
            } else {
                break;
            }
        }
    }
}

static void chess_valid_moves_queen(square_t square, char piece_color) {
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

            char piece = chess_square_get(target);
            if (piece == CHESS_NONE) {
                ds_dynamic_array_append(&moves, &target);
            } else if ((piece & COLOR_FLAG) != piece_color) {
                ds_dynamic_array_append(&moves, &target);
                break;
            } else {
                break;
            }
        }
    }
}

static void chess_valid_moves_king(square_t square, char piece_color) {
    int rank_diffs[] = {1, 1, -1, -1, 1, -1, 0, 0};
    int file_diffs[] = {1, -1, 1, -1, 0, 0, 1, -1};

    for (unsigned int i = 0; i < 8; i++) {
        int rank_diff = rank_diffs[i];
        int file_diff = file_diffs[i];

        square_t target = { .file = square.file + file_diff, .rank = square.rank + rank_diff };
        if (target.file < 0 || target.file >= CHESS_WIDTH || target.rank < 0 || target.rank >= CHESS_HEIGHT) {
            continue;
        }

        char piece = chess_square_get(target);
        if (piece == CHESS_NONE) {
            ds_dynamic_array_append(&moves, &target);
        } else if ((piece & COLOR_FLAG) != piece_color) {
            ds_dynamic_array_append(&moves, &target);
        }
    }
}

static void chess_valid_moves(square_t square) {
    ds_dynamic_array_clear(&moves);
    char piece = chess_square_get(square);
    char piece_type = piece & PIECE_FLAG;
    char piece_color = piece & COLOR_FLAG;

    switch (piece_type) {
        case CHESS_PAWN:
            return chess_valid_moves_pawn(square, piece_color);
        case CHESS_KNIGHT:
            return chess_valid_moves_knight(square, piece_color);
        case CHESS_BISHOP:
            return chess_valid_moves_bishop(square, piece_color);
        case CHESS_ROOK:
            return chess_valid_moves_rook(square, piece_color);
        case CHESS_QUEEN:
            return chess_valid_moves_queen(square, piece_color);
        case CHESS_KING:
            return chess_valid_moves_king(square, piece_color);
        default:
            return;
    }
}

static void chess_apply_move(square_t square) {
    for (unsigned int i = 0; i < moves.count; i++) {
        square_t *move = NULL;
        DS_UNREACHABLE(ds_dynamic_array_get_ref(&moves, i, (void **)&move));
        if (move->rank == square.rank && move->file == square.file) {
            chess_square_set(square, chess_square_get(selected_square));
            chess_square_set(selected_square, CHESS_NONE);

            return;
        }
    }
}

void init(void *memory, unsigned long size) {
    js_clear_canvas();

    DS_INIT_ALLOCATOR(&allocator, memory, size);

    ds_dynamic_array_init_allocator(&moves, sizeof(ivec2), &allocator);

    ds_string_slice fen = DS_STRING_SLICE(CHESS_START);
    chess_load_fen(fen);

    chess_print_board();
}

void tick(float deltaTime) {
    js_clear_canvas();

    chess_print_board();

    ivec2 mouse_px = {0};
    js_hover_px(&mouse_px);
    square_t square = {0};
    px_to_square(&mouse_px, &square);

    if (js_mouse_down()) {
        if (is_selected) {
            is_selected = 0;
            chess_apply_move(square);
            ds_dynamic_array_clear(&moves);
        } else if (chess_square_get(square) != CHESS_NONE) {
            is_selected = 1;
            selected_square = square;
            chess_valid_moves(square);
        }
    }
}
