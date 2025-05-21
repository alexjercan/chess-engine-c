#include "stdlib.h"
#include "raylib.h"
#include "chess.h"
#include "game.h"
#include "ds.h"
#include <dlfcn.h>

#define SUBCMD_GAME "game"
#define SUBCMD_COUNT_POSITIONS "count-positions"
#define SUBCMD_MOVE "move"

typedef struct arguments_t {
    char *subcmd;
    int depth;
    char *player1;
    char *player2;
} arguments_t;

init_fn init_player1 = NULL;
init_fn init_player2 = NULL;
move_fn move_player1 = NULL;
move_fn move_player2 = NULL;

extern void init_player1_fn(void *memory, unsigned long size) { return init_player1(memory, size); }
extern void init_player2_fn(void *memory, unsigned long size) { return init_player2(memory, size); }

extern void move_player1_fn(const chess_state_t *state, move_t *choices, int count, int *index) { return move_player1(state, choices, count, index); }
extern void move_player2_fn(const chess_state_t *state, move_t *choices, int count, int *index) { return move_player2(state, choices, count, index); }

void parse_arguments(int argc, char **argv, arguments_t *args) {
    ds_argparse_parser parser = {0};
    ds_argparse_parser_init(
        &parser,
        "chess-engine",
        "Chess Engine in C",
        "0.1"
    );

    ds_argparse_add_argument(&parser, (ds_argparse_options){
        .short_name = 's',
        .long_name = "subcmd",
        .description = "The subcommand to use with the engine: `game`, `count-positions`, `move`. Default: `game`",
        .type = ARGUMENT_TYPE_POSITIONAL,
        .required = false,
    });

    ds_argparse_add_argument(&parser, (ds_argparse_options){
        .short_name = 'd',
        .long_name = "depth",
        .description = "The depth to use with the engine. Default: `0`.",
        .type = ARGUMENT_TYPE_VALUE,
        .required = false,
    });

    ds_argparse_add_argument(&parser, (ds_argparse_options){
        .short_name = 'a',
        .long_name = "player1",
        .description = "the strategy for player 1, a path to a DLL",
        .type = ARGUMENT_TYPE_POSITIONAL,
        .required = false,
    });

    ds_argparse_add_argument(&parser, (ds_argparse_options){
        .short_name = 'b',
        .long_name = "player2",
        .description = "the strategy for player 2, a path to a DLL",
        .type = ARGUMENT_TYPE_POSITIONAL,
        .required = false,
    });

    DS_UNREACHABLE(ds_argparse_parse(&parser, argc, argv));

    args->subcmd = ds_argparse_get_value_or_default(&parser, "subcmd", SUBCMD_GAME);
    args->depth = atoi(ds_argparse_get_value_or_default(&parser, "depth", "0"));
    args->player1 = ds_argparse_get_value(&parser, "player1");
    args->player2 = ds_argparse_get_value(&parser, "player2");

    ds_argparse_parser_free(&parser);
}

int load_move_function(const char *player, move_fn *move_player, init_fn *init_player) {
    void *strat = dlopen(player, RTLD_NOW);
    if (strat == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    *move_player = (move_fn)dlsym(strat, "chess_move");
    if (move_player == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    *init_player = (init_fn)dlsym(strat, "chess_init");
    if (init_player == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    return 0;
}

int game(arguments_t args) {
    if (load_move_function(args.player1, &move_player1, &init_player1) != 0) {
        return 1;
    }

    if (load_move_function(args.player2, &move_player2, &init_player2) != 0) {
        return 1;
    }

    init_player1_fn(NULL, 0);
    init_player2_fn(NULL, 0);

    init(NULL, 0);
    while (!WindowShouldClose()) {
        tick(GetFrameTime());
    }

    CloseWindow();

    return 0;
}

int count_positions(int depth) {
    chess_state_t state = {0};

    ds_string_slice fen = DS_STRING_SLICE(CHESS_START);
    chess_init_fen(&state, fen);

    perft_t perft = {0};
    chess_count_positions(&state, depth, &perft);
    DS_LOG_INFO("Depth: %d => Positions: %d Captures: %d E.p.: %d Castles: %d "
                "Promotions: %d Checks: %d Checkmates: %d",
                depth, perft.nodes, perft.captures, perft.enp, perft.castles,
                perft.promote, perft.checks, perft.checkmates);

    return 0;
}

int move(arguments_t args) {
    if (load_move_function(args.player1, &move_player1, &init_player1) != 0) {
        return 1;
    }

    init_player1_fn(NULL, 0);

    int index = -1;
    chess_state_t state = {0};
    ds_dynamic_array moves = {0}; /* move_t */

    ds_string_slice fen = DS_STRING_SLICE(CHESS_START);
    chess_init_fen(&state, fen);

    ds_dynamic_array_init_allocator(&moves, sizeof(move_t), NULL);
    chess_generate_moves(&state, &moves);

    move_player1_fn(&state, moves.items, moves.count, &index);

    move_t *move = NULL;
    ds_dynamic_array_get_ref(&moves, index, (void **)&move);

    DS_LOG_INFO("Move: %d,%d %d,%d", move->start.file, move->start.rank, move->end.file, move->end.rank);

    ds_dynamic_array_free(&moves);

    return 0;
}

int main(int argc, char **argv) {
    arguments_t args = {0};
    parse_arguments(argc, argv, &args);

    if (DS_STRCMP(args.subcmd, SUBCMD_GAME) == 0) {
        return game(args);
    } else if (DS_STRCMP(args.subcmd, SUBCMD_COUNT_POSITIONS) == 0) {
        return count_positions(args.depth);
    } else if (DS_STRCMP(args.subcmd, SUBCMD_MOVE) == 0) {
        return move(args);
    }

    return 1;
}
