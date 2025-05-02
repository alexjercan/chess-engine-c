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

function parseCString(addr: number): string {
    const memory = new Uint8Array(
        (w!.instance.exports.memory as WebAssembly.Memory).buffer
    );

    let length = 0;
    while (memory[addr + length] != 0) {
        length += 1;
    }

    return new TextDecoder("utf-8").decode(memory.slice(addr, addr + length));
}

function dumpCString(value: string, addr: number): void {
    const memory = new Uint8Array(
        (w!.instance.exports.memory as WebAssembly.Memory).buffer
    );

    memory.set(new TextEncoder().encode(value), addr);
    memory[addr + value.length] = 0;
}

function parseCInt(addr: number): number {
    const memory = new Uint8Array(
        (w!.instance.exports.memory as WebAssembly.Memory).buffer
    );

    return (memory[addr + 0] << 0 | memory[addr + 1] << 8 | memory[addr + 2] << 16 | memory[addr + 3] << 24);
}

function dumpCInt(value: number, addr: number): void {
    const memory = new Uint8Array(
        (w!.instance.exports.memory as WebAssembly.Memory).buffer
    );

    memory[addr + 0] = (value & 0x000000ff) >> 0;
    memory[addr + 1] = (value & 0x0000ff00) >> 8;
    memory[addr + 2] = (value & 0x00ff0000) >> 16;
    memory[addr + 3] = (value & 0xff000000) >> 24;
}

function formatString(format: string, restAddr: number): string {
    let final = "";
    let argAddr = restAddr;
    for (let i = 0; i < format.length; i++) {
        if (format[i] === "%") {
            if (format[i+1] === "s") {
                final = final + parseCString(parseCInt(argAddr));
            } else if (format[i+1] === "d") {
                final = final + parseCInt(argAddr);
            }

            i = i + 1;
            argAddr += 4;
        } else {
            final = final + format[i];
        }
    }

    return final;
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
        js_format: (bufferAddr: number, formatAddr: number, restAddr: number): number => {
            const format = parseCString(formatAddr);

            let final = formatString(format, restAddr);

            if (bufferAddr !== 0) {
                dumpCString(final, bufferAddr);
            }

            return final.length;
        },
        js_log_cstr: (formatAddr: number, restAddr: number): void => {
            const format = parseCString(formatAddr);

            let final = formatString(format, restAddr);

            console.log(final);
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
        js_canvas_hover_px: (positionAddr: number): void => {
            dumpCInt(xCoordinate, positionAddr);
            dumpCInt(yCoordinate, positionAddr + 4);
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
