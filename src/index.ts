type SumFunction = (a: number, b: number) => number;

let w: WebAssembly.WebAssemblyInstantiatedSource | null = null;

WebAssembly.instantiateStreaming(fetch("main.wasm"), {
    env: {
        js_log_cstr: (): void => {

        }
    },
}).then((value) => {
    w = value;

    let sum = value.instance.exports.sum as SumFunction;

    console.log(sum(10, 59));
});
