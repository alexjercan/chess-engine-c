import { ASSETS, IMAGE_WIDTH } from "./assets";

type MovePlayerFunction = (state: number, moves: number, count: number, index: number) => void;
type UtilMallocFunction = (size: number) => number;
type UtilFreeFunction = (ptr: number) => void;
type ChessStateSizeFunction = () => number;
type ChessMoveSizeFunction = () => number;

interface WasmModule {
    wasm: WebAssembly.WebAssemblyInstantiatedSource | null;

    load(wasm: WebAssembly.WebAssemblyInstantiatedSource): void;
}

function make_environment(...modules: WasmModule[]) {
    return new Proxy(
        {},
        {
            get(_target, prop, _receiver) {
                for (const m of modules) {
                    if ((m as any)[prop] !== undefined) {
                        return (m as any)[prop].bind(m);
                    }
                }
                return (...args: any[]) => {
                    throw new Error(`NOT IMPLEMENTED: ${String(prop)} ${args}`);
                };
            },
        }
    );
}

export async function create(
    wasmPath: string,
    ...modules: WasmModule[]
): Promise<WebAssembly.WebAssemblyInstantiatedSource> {
    const wasm = await WebAssembly.instantiateStreaming(fetch(wasmPath), {
        env: make_environment(...modules),
    });

    for (const m of modules) {
        m.load(wasm);
    }

    return wasm;
}

function parseColor(color: number): string {
    const r = (color & 0x0000ff) >> 0;
    const g = (color & 0x00ff00) >> 8;
    const b = (color & 0xff0000) >> 16;
    const a = 0xff;

    return `rgba(${r}, ${g}, ${b}, ${a})`;
}

function parseCString(memory: Uint8Array, addr: number): string {
    let length = 0;
    while (memory[addr + length] != 0) {
        length += 1;
    }

    return new TextDecoder("utf-8").decode(memory.slice(addr, addr + length));
}

function dumpCString(memory: Uint8Array, value: string, addr: number): void {
    memory.set(new TextEncoder().encode(value), addr);
    memory[addr + value.length] = 0;
}

function parseCInt(memory: Uint8Array, addr: number): number {
    return (
        (memory[addr + 0] << 0) |
        (memory[addr + 1] << 8) |
        (memory[addr + 2] << 16) |
        (memory[addr + 3] << 24)
    );
}

function dumpCInt(memory: Uint8Array, value: number, addr: number): void {
    memory[addr + 0] = (value & 0x000000ff) >> 0;
    memory[addr + 1] = (value & 0x0000ff00) >> 8;
    memory[addr + 2] = (value & 0x00ff0000) >> 16;
    memory[addr + 3] = (value & 0xff000000) >> 24;
}

function parseCFloat(memory: Uint8Array, addr: number): number {
    const buffer = new ArrayBuffer(4);
    const view = new Uint8Array(buffer);

    for (let i = 0; i < 4; i++) {
        view[i] = memory[addr + i];
    }

    return new Float32Array(buffer)[0];
}

function formatString(
    memory: Uint8Array,
    format: string,
    restAddr: number
): string {
    let final = "";
    let argAddr = restAddr;
    for (let i = 0; i < format.length; i++) {
        if (format[i] === "%") {
            if (format[i + 1] === "s") {
                final =
                    final + parseCString(memory, parseCInt(memory, argAddr));
            } else if (format[i + 1] === "d") {
                final = final + parseCInt(memory, argAddr);
            } else if (format[i + 1] === "f") {
                final = final + parseCFloat(memory, argAddr);
            }

            i = i + 1;
            argAddr += 4;
        } else {
            final = final + format[i];
        }
    }

    return final;
}

const root = document.createElement("div");
root.id = "root";
document.body.appendChild(root);

const canvas = document.createElement("canvas");

let xCoordinate: number = -1;
let yCoordinate: number = -1;
let mouseDown: boolean = false;
let mouseUp: boolean = false;
let imageIds: string[] = [];
let soundIds: string[] = [];

let buttons: number[] = [];

canvas.addEventListener("mousemove", (event) => {
    const rect = canvas.getBoundingClientRect();
    xCoordinate = event.clientX - rect.left;
    yCoordinate = event.clientY - rect.top;
});
canvas.addEventListener("mousedown", (event) => {
    const rect = canvas.getBoundingClientRect();
    xCoordinate = event.clientX - rect.left;
    yCoordinate = event.clientY - rect.top;
    mouseDown = true;

    buttons.push(event.button);
});
canvas.addEventListener("mouseup", (event) => {
    const rect = canvas.getBoundingClientRect();
    xCoordinate = event.clientX - rect.left;
    yCoordinate = event.clientY - rect.top;
    mouseUp = true;

    const index = buttons.indexOf(event.button);
    if (index > -1) {
        buttons.splice(index, 1);
    }
});

let ctx: CanvasRenderingContext2D = canvas.getContext("2d")!;

export class RaylibJS implements WasmModule {
    wasm: WebAssembly.WebAssemblyInstantiatedSource | null = null;

    constructor() {}

    load(wasm: WebAssembly.WebAssemblyInstantiatedSource): void {
        this.wasm = wasm;
    }

    InitWindow(width: number, height: number, titleAddr: number): void {
        const memory = new Uint8Array(
            (this.wasm.instance.exports.memory as WebAssembly.Memory).buffer
        );

        const title = parseCString(memory, titleAddr);
        document.title = title;

        root.innerHTML = "";

        canvas.width = width;
        canvas.height = height;
        root.appendChild(canvas);

        ctx = canvas.getContext("2d");
    }

    InitAudioDevice(): void {}

    LoadSoundInternal(fileNameAddr: number, idAddr: number): void {
        const memory = new Uint8Array(
            (this.wasm!.instance.exports.memory as WebAssembly.Memory).buffer
        );

        const fileName = parseCString(memory, fileNameAddr);

        const audioPath = ASSETS[fileName];
        let index = soundIds.findIndex((path) => path === audioPath);

        if (index === -1) {
            soundIds.push(audioPath);
            index = soundIds.length - 1;
        }

        dumpCInt(memory, index, idAddr);
    }

    PlaySoundInternal(id: number): void {
        const audio = new Audio();
        audio.src = soundIds[id];

        audio.play();
    }

    ClearBackground(colorAddr: number): void {
        const memory = new Uint8Array(
            (this.wasm!.instance.exports.memory as WebAssembly.Memory).buffer
        );
        const color = parseCInt(memory, colorAddr);
        ctx.fillStyle = parseColor(color);
        ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);
    }

    BeginDrawing(): void {}

    EndDrawing(): void {}

    DrawRectangle(
        posX: number,
        posY: number,
        width: number,
        height: number,
        colorAddr: number
    ): void {
        const memory = new Uint8Array(
            (this.wasm!.instance.exports.memory as WebAssembly.Memory).buffer
        );
        const color = parseCInt(memory, colorAddr);
        ctx.fillStyle = parseColor(color);
        ctx.fillRect(posX, posY, width, height);
    }

    DrawCircle(
        centerX: number,
        centerY: number,
        radius: number,
        colorAddr: number
    ): void {
        const memory = new Uint8Array(
            (this.wasm!.instance.exports.memory as WebAssembly.Memory).buffer
        );
        const color = parseCInt(memory, colorAddr);
        ctx.fillStyle = parseColor(color);
        ctx.beginPath();
        ctx.arc(centerX, centerY, radius, 0, Math.PI * 2);
        ctx.fill();
    }

    IsMouseButtonPressed(button: number): number {
        if (mouseDown && buttons.includes(button)) {
            mouseDown = false;
            return 1;
        }

        return 0;
    }

    GetMousePositionInternal(xAddr: number, yAddr: number): void {
        const memory = new Uint8Array(
            (this.wasm!.instance.exports.memory as WebAssembly.Memory).buffer
        );

        dumpCInt(memory, xCoordinate, xAddr);
        dumpCInt(memory, yCoordinate, yAddr);
    }

    LoadTextureInternal(
        fileNameAddr: number,
        idAddr: number,
        widthAddr: number,
        heightAddr: number,
        mipmapsAddr: number,
        formatAddr: number
    ): void {
        const memory = new Uint8Array(
            (this.wasm!.instance.exports.memory as WebAssembly.Memory).buffer
        );

        const fileName = parseCString(memory, fileNameAddr);

        const imagePath = ASSETS[fileName];
        let index = imageIds.findIndex((path) => path === imagePath);

        if (index === -1) {
            imageIds.push(imagePath);
            index = imageIds.length - 1;
        }

        dumpCInt(memory, index, idAddr);

        dumpCInt(memory, IMAGE_WIDTH, widthAddr);
        dumpCInt(memory, IMAGE_WIDTH, heightAddr);
        dumpCInt(memory, 1, mipmapsAddr);
        dumpCInt(memory, 0, formatAddr);
    }

    DrawTextureEx(
        textureAddr: number,
        vector2Addr: number,
        rotation: number,
        scale: number,
        colorAddr: number
    ): void {
        const memory = new Uint8Array(
            (this.wasm!.instance.exports.memory as WebAssembly.Memory).buffer
        );

        const id = parseCInt(memory, textureAddr);
        const x = parseCFloat(memory, vector2Addr);
        const y = parseCFloat(memory, vector2Addr + 4);

        const image = new Image();
        image.src = imageIds[id];

        ctx.drawImage(image, x, y, IMAGE_WIDTH * scale, IMAGE_WIDTH * scale);
    }

    MeasureText(textAddr: number, fontSize: number): number {
        const memory = new Uint8Array(
            (this.wasm!.instance.exports.memory as WebAssembly.Memory).buffer
        );
        const text = parseCString(memory, textAddr);
        ctx.font = `${fontSize}px sans-serif`;
        const metrics = ctx.measureText(text);
        return metrics.width;
    }

    DrawText(
        textAddr: number,
        posX: number,
        posY: number,
        fontSize: number,
        colorAddr: number
    ): void {
        const memory = new Uint8Array(
            (this.wasm!.instance.exports.memory as WebAssembly.Memory).buffer
        );
        const text = parseCString(memory, textAddr);
        const color = parseCInt(memory, colorAddr);
        ctx.fillStyle = parseColor(color);
        ctx.font = `${fontSize}px sans-serif`;
        ctx.fillText(text, posX, posY);
    }

    StringFormat(
        bufferAddr: number,
        formatAddr: number,
        restAddr: number
    ): number {
        const memory = new Uint8Array(
            (this.wasm!.instance.exports.memory as WebAssembly.Memory).buffer
        );

        const format = parseCString(memory, formatAddr);
        let final = formatString(memory, format, restAddr);

        if (bufferAddr !== 0) {
            dumpCString(memory, final, bufferAddr);
        }

        return final.length;
    }

    ConsoleLog(formatAddr: number, restAddr: number): void {
        const memory = new Uint8Array(
            (this.wasm!.instance.exports.memory as WebAssembly.Memory).buffer
        );

        const format = parseCString(memory, formatAddr);
        let final = formatString(memory, format, restAddr);

        console.log(final);
    }

    memset(dest: number, c: number, sizeof: number): void {
        console.log("memset", dest, c, sizeof);

        const memory = new Uint8Array(
            (this.wasm!.instance.exports.memory as WebAssembly.Memory).buffer
        );

        for (let i = 0; i < sizeof; i++) {
            memory[dest + i] = c;
        }
    }

    rand(): number {
        return Math.floor(Math.random() * 2147483647);
    }
}

export class GameJS implements WasmModule {
    wasm: WebAssembly.WebAssemblyInstantiatedSource | null = null;
    player1: WebAssembly.WebAssemblyInstantiatedSource;
    player2: WebAssembly.WebAssemblyInstantiatedSource;

    constructor(
        player1: WebAssembly.WebAssemblyInstantiatedSource,
        player2: WebAssembly.WebAssemblyInstantiatedSource
    ) {
        this.player1 = player1;
        this.player2 = player2;
    }

    load(wasm: WebAssembly.WebAssemblyInstantiatedSource): void {
        this.wasm = wasm;
    }

    move_player_fn(player: WebAssembly.WebAssemblyInstantiatedSource, stateAddr: number, movesAddr: number, count: number, indexAddr: number): void {
        const state_size = (
            this.wasm.instance.exports
                .chess_state_size as ChessStateSizeFunction
        )();
        const move_size = (
            this.wasm.instance.exports
                .chess_move_size as ChessMoveSizeFunction
        )();
        const moves_size = move_size * count;

        const wasmMemoryBuffer = new Uint8Array(
            (this.wasm!.instance.exports.memory as WebAssembly.Memory).buffer
        );
        const playerMemoryBuffer = new Uint8Array(
            (player.instance.exports.memory as WebAssembly.Memory).buffer
        );

        const playerStateAddr = (
            player.instance.exports.util_malloc as UtilMallocFunction
        )(state_size);
        const playerMovesAddr = (
            player.instance.exports.util_malloc as UtilMallocFunction
        )(moves_size);
        const playerIndexAddr = (
            player.instance.exports.util_malloc as UtilMallocFunction
        )(4);

        playerMemoryBuffer.set(
            wasmMemoryBuffer.subarray(stateAddr, stateAddr + state_size),
            playerStateAddr
        );
        playerMemoryBuffer.set(
            wasmMemoryBuffer.subarray(movesAddr, movesAddr + moves_size),
            playerMovesAddr
        );
        playerMemoryBuffer.set(
            wasmMemoryBuffer.subarray(indexAddr, indexAddr + 4),
            playerIndexAddr
        );

        let chess_move = player.instance.exports
            .chess_move as MovePlayerFunction;
        chess_move(playerStateAddr, playerMovesAddr, count, playerIndexAddr);

        wasmMemoryBuffer.set(
            playerMemoryBuffer.subarray(playerIndexAddr, playerIndexAddr + 4),
            indexAddr
        );
        (player.instance.exports.util_free as UtilFreeFunction)(playerStateAddr);
        (player.instance.exports.util_free as UtilFreeFunction)(playerMovesAddr);
        (player.instance.exports.util_free as UtilFreeFunction)(playerIndexAddr);
    }

    move_player1_fn(stateAddr: number, movesAddr: number, count: number, indexAddr: number): void {
        const player = this.player1;
        this.move_player_fn(player, stateAddr, movesAddr, count, indexAddr);
    }

    move_player2_fn(stateAddr: number, movesAddr: number, count: number, indexAddr: number): void {
        const player = this.player2;
        this.move_player_fn(player, stateAddr, movesAddr, count, indexAddr);
    }
}
