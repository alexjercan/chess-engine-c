#include "stdlib.h"
#include "game.h"
#include "raylib.h"

int main(void) {
    init(NULL, 0);
    while (!WindowShouldClose()) {
        tick(GetFrameTime());
    }

    CloseWindow();

    return 0;
}
