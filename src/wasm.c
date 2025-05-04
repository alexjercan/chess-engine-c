#define DS_LIST_ALLOCATOR_IMPLEMENTATION
#include "chess.h"
#include "wasm.h"

DS_ALLOCATOR allocator;
chess_board_t board;
square_t selected_square = {0};
int is_selected = 0;
ds_dynamic_array moves = {0};

#define ds_string_builder_append(sb, format, ...)                              \
    do {                                                                       \
        int needed = js_format(NULL, format, __VA_ARGS__);                     \
        char *buffer = DS_MALLOC((sb)->items.allocator, needed + 1);           \
        js_format(buffer, format, __VA_ARGS__);                                \
        buffer[needed] = '\0';                                                 \
        ds_dynamic_array_append_many(&(sb)->items, (void **)buffer, needed);   \
        DS_FREE((sb)->items.allocator, buffer);                                \
    } while (0)

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

void chess_print_board(chess_board_t *board) {
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
            char piece = chess_square_get(board, (square_t){.rank = rank, .file = file});
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

void init(void *memory, unsigned long size) {
    js_clear_canvas();

    DS_INIT_ALLOCATOR(&allocator, memory, size);

    ds_dynamic_array_init_allocator(&moves, sizeof(ivec2), &allocator);

    ds_string_slice fen = DS_STRING_SLICE(CHESS_START);
    chess_init_fen(&board, fen);

    chess_print_board(&board);
}

void tick(float deltaTime) {
    js_clear_canvas();

    chess_print_board(&board);

    ivec2 mouse_px = {0};
    js_hover_px(&mouse_px);
    square_t square = {0};
    px_to_square(&mouse_px, &square);

    if (js_mouse_down()) {
        if (is_selected) {
            is_selected = 0;
            chess_apply_move(&board, square, selected_square, moves);
            ds_dynamic_array_clear(&moves);
        } else if (chess_square_get(&board, square) != CHESS_NONE) {
            is_selected = 1;
            selected_square = square;
            chess_valid_moves(&board, square, &moves);
        }
    }
}
