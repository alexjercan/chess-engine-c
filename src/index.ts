import { RaylibJS, GameJS, create } from "./raylib";

const FPS: number = 60;
const MEMORY_SIZE: number = 8192;

type InitPlayerFunction = (memory: number, size: number) => void;
type InitFunction = (memory: number, size: number) => void;
type TickFunction = (deltaTime: number) => void;

async function main() {
    const urlParams = new URLSearchParams(window.location.search);
    const player1Wasm = (urlParams.get("player1") || "human") + ".wasm";
    const player2Wasm = (urlParams.get("player2") || "human") + ".wasm";

    let player1 = await create(player1Wasm, new RaylibJS());
    const player1Memory = (
        player1.instance.exports.__heap_base as WebAssembly.Global
    ).value;
    let player1Init = player1.instance.exports.chess_init as InitPlayerFunction;
    player1Init(player1Memory, MEMORY_SIZE);

    let player2 = await create(player2Wasm, new RaylibJS());
    const player2Memory = (
        player2.instance.exports.__heap_base as WebAssembly.Global
    ).value;
    let player2Init = player2.instance.exports.chess_init as InitPlayerFunction;
    player2Init(player2Memory, MEMORY_SIZE);

    let raylib = new RaylibJS();
    let gamejs = new GameJS(player1, player2);

    let wasm = await create("main.wasm", raylib, gamejs);
    const memory = (
        wasm.instance.exports.__heap_base as WebAssembly.Global
    ).value;
    let init = wasm.instance.exports.init as InitFunction;
    init(memory, MEMORY_SIZE);

    let fpsInterval = 1000 / FPS;
    let then = Date.now();
    function gameLoop() {
        requestAnimationFrame(gameLoop);

        let now = Date.now();
        let elapsed = now - then;

        if (elapsed > fpsInterval) {
            then = now - (elapsed % fpsInterval);

            let tick = wasm.instance.exports.tick as TickFunction;
            tick(elapsed / 1000);
        }
    }

    requestAnimationFrame(gameLoop);
}

main().catch((e) => {
    console.error("Error in main function", e);
});
