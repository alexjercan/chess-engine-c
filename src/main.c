#include "stdlib.h"
#include "raylib.h"
#include "chess.h"
#include "game.h"
#include "ds.h"
#include <dlfcn.h>

#define SUBCMD_GAME "game"
#define SUBCMD_COUNT_POSITIONS "count-positions"

typedef struct arguments_t {
    char *subcmd;
    int depth;
    char *player1;
    char *player2;
} arguments_t;

init_t init_player1 = NULL;
init_t init_player2 = NULL;
move_t move_player1 = NULL;
move_t move_player2 = NULL;

extern void init_player1_fn(void *memory, unsigned long size) { return init_player1(memory, size); }
extern void init_player2_fn(void *memory, unsigned long size) { return init_player2(memory, size); }

extern void move_player1_fn(chess_state_t *board) { return move_player1(board); }
extern void move_player2_fn(chess_state_t *board) { return move_player2(board); }


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
        .description = "The subcommand to use with the engine: `game`, `count-positions`. Default: `game`",
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

int load_move_function(arguments_t args) {
    void *strat1 = dlopen(args.player1, RTLD_NOW);
    if (strat1 == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    move_player1 = (move_t)dlsym(strat1, "chess_move");
    if (move_player1 == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    init_player1 = (init_t)dlsym(strat1, "chess_init");
    if (init_player1 == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    void *strat2 = dlopen(args.player2, RTLD_NOW);
    if (strat2 == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    move_player2 = (move_t)dlsym(strat2, "chess_move");
    if (move_player2 == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    init_player2 = (init_t)dlsym(strat2, "chess_init");
    if (init_player2 == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    return 0;
}

int game(arguments_t args) {
    if (load_move_function(args) != 0) {
        return 1;
    }

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
    chess_count_positions(&state, CHESS_WHITE, depth, &perft);
    DS_LOG_INFO("Depth: %d => Positions: %d Captures: %d E.p.: %d Castles: %d "
                "Promotions: %d Checks: %d Checkmates: %d",
                depth, perft.nodes, perft.captures, perft.enp, perft.castles,
                perft.promote, perft.checks, perft.checkmates);

    return 0;
}

int main(int argc, char **argv) {
    arguments_t args = {0};
    parse_arguments(argc, argv, &args);

    if (DS_STRCMP(args.subcmd, SUBCMD_GAME) == 0) {
        return game(args);
    } else if (DS_STRCMP(args.subcmd, SUBCMD_COUNT_POSITIONS) == 0) {
        return count_positions(args.depth);
    }

    return 1;
}
