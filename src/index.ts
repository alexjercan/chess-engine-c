import { PIECES } from "./piece";

const FPS: number = 60;
const MEMORY_SIZE: number = 8192;

const CANVAS_WIDTH = 800;
const CANVAS_HEIGHT = 800;

const root = document.createElement("div");
root.id = "root";
document.body.appendChild(root);

const canvas = document.createElement("canvas");
canvas.width = CANVAS_WIDTH;
canvas.height = CANVAS_HEIGHT;
root.appendChild(canvas);

let xCoordinate = -1;
let yCoordinate = -1;
let clicked = false;
canvas.addEventListener("mousemove", (event) => {
    const rect = canvas.getBoundingClientRect();
    xCoordinate = event.clientX - rect.left;
    yCoordinate = event.clientY - rect.top;
});
canvas.addEventListener("click", (event) => {
    const rect = canvas.getBoundingClientRect();
    xCoordinate = event.clientX - rect.left;
    yCoordinate = event.clientY - rect.top;
    clicked = true;
});

const ctx = canvas.getContext("2d");

type InitFunction = (memory: number, size: number) => void;
type TickFunction = (deltaTime: number) => void;

let w: WebAssembly.WebAssemblyInstantiatedSource | null = null;

function intToArray(i: number): Uint8Array {
    return Uint8Array.of(
        (i & 0x000000ff) >> 0,
        (i & 0x0000ff00) >> 8,
        (i & 0x00ff0000) >> 16,
        (i & 0xff000000) >> 24
    );
}

WebAssembly.instantiateStreaming(fetch("main.wasm"), {
    env: {
        memory: new WebAssembly.Memory({ initial: MEMORY_SIZE }),
        js_width: (): number => {
            return CANVAS_WIDTH;
        },
        js_height: (): number => {
            return CANVAS_HEIGHT;
        },
        js_random: (a: number, b: number): number => {
            return Math.random() * (b - a) + a;
        },
        js_clear_canvas: (): void => {
            ctx!.clearRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);
        },
        js_fill_rect: (
            x: number,
            y: number,
            w: number,
            h: number,
            color: number
        ): void => {
            ctx!.fillStyle = `#${color.toString(16).padStart(6, "0")}`;
            ctx!.fillRect(x, y, w, h);
        },
        js_draw_outline: (
            x: number,
            y: number,
            w: number,
            h: number,
            color: number
        ): void => {
            ctx!.strokeStyle = `#${color.toString(16).padStart(6, "0")}`;
            ctx!.strokeRect(x, y, w, h);
        },
        js_fill_piece: (
            x: number,
            y: number,
            w: number,
            h: number,
            piece: number
        ): void => {
            const image = new Image();
            image.src = PIECES[piece - 1];
            ctx.drawImage(image, x, y, w, h);
        },
        js_log_cstr: (message: number): void => {
            const memory = new Uint8Array(
                (w!.instance.exports.memory as WebAssembly.Memory).buffer
            );

            let length = 0;
            while (memory[message + length] != 0) {
                length += 1;
            }

            const msg = new TextDecoder("utf-8").decode(
                memory.slice(message, message + length)
            );
            console.log(msg);
        },
        js_canvas_hover_px: (position: number): void => {
            const memory = new Uint8Array(
                (w!.instance.exports.memory as WebAssembly.Memory).buffer
            );

            const x = intToArray(xCoordinate);
            const y = intToArray(yCoordinate);
            for (let i = 0; i < 4; i++) {
                memory[position + i] = x[i];
                memory[position + i + 4] = y[i];
            }
        },
        js_canvas_clicked: (): number => {
            if (clicked) {
                clicked = false;
                return 1;
            }

            return 0;
        },
    },
})
    .then((value) => {
        let fpsInterval = 1000 / FPS;

        w = value;

        let init = w.instance.exports.init as InitFunction;
        let tick = w.instance.exports.tick as TickFunction;

        const memory = (w.instance.exports.__heap_base as WebAssembly.Global)
            .value;

        init(memory, MEMORY_SIZE);

        let then = Date.now();

        function gameLoop() {
            requestAnimationFrame(gameLoop);

            let now = Date.now();
            let elapsed = now - then;

            if (elapsed > fpsInterval) {
                then = now - (elapsed % fpsInterval);

                tick(elapsed);
            }
        }

        requestAnimationFrame(gameLoop);
    })
    .catch((e) => {
        console.error("Error instantiating the WebAssembly module", e);
    });
