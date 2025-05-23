const path = require("path");
const fs = require("fs");
const HtmlWebpackPlugin = require("html-webpack-plugin");
const { execSync } = require("child_process");

class ClangPlugin {
    constructor(options) {
        this.options = options || {};
    }

    apply(compiler) {
        compiler.hooks.afterEmit.tapAsync(
            "ClangPlugin",
            (compilation, callback) => {
                const cFilesPath = this.options.cFiles.map((cFile) =>
                    path.join(__dirname, cFile)
                );
                const wasmFilePath = path.join(
                    compilation.outputOptions.path,
                    this.options.outputFileName
                );

                const buildDir = path.dirname(wasmFilePath);
                if (!fs.existsSync(buildDir)) {
                    fs.mkdirSync(buildDir, { recursive: true });
                }

                const clangCommand = [
                    "clang-19",
                    "--target=wasm32",
                    "--no-standard-libraries",
                    "-Wl,--export-all",
                    "-Wl,--allow-undefined",
                    "-Wl,--no-entry",
                    "-DDS_NO_STDLIB",
                    "-DDS_LIST_ALLOCATOR",
                    "-DDS_NO_TERMINAL_COLORS",
                    "-DDS_DA_INIT_CAPACITY=8",
                    '-DASSETS_FOLDER=""',
                    `-o ${wasmFilePath}`,
                    ...cFilesPath,
                ].join(" ");

                try {
                    execSync(clangCommand, { stdio: "inherit", shell: true });
                    console.log("C code compiled to WebAssembly successfully.");
                } catch (error) {
                    console.error(
                        "Error compiling C code to WebAssembly:",
                        error.message
                    );
                }

                callback();
            }
        );
    }
}

const config = {
    entry: "./src/index.ts",
    output: {
        path: path.resolve(__dirname, "dist"),
        filename: "index.js",
        assetModuleFilename: "assets/[name][ext]",
        clean: true,
    },
    plugins: [
        new HtmlWebpackPlugin({
            template: "src/index.html",
        }),
        new ClangPlugin({
            cFiles: ["src/game.c", "src/chess.c", "src/util.c", "src/ds.c"],
            outputFileName: "main.wasm",
        }),
        new ClangPlugin({
            cFiles: ["src/human.c", "src/chess.c", "src/util.c", "src/ds.c"],
            outputFileName: "human.wasm",
        }),
        new ClangPlugin({
            cFiles: ["src/random.c", "src/chess.c", "src/util.c", "src/ds.c"],
            outputFileName: "random.wasm",
        }),
        new ClangPlugin({
            cFiles: ["src/minmax.c", "src/chess.c", "src/util.c", "src/ds.c"],
            outputFileName: "minmax.wasm",
        }),
    ],
    resolve: {
        extensions: [".ts", ".tsx", ".js", ".wasm"],
    },
    module: {
        rules: [
            {
                test: /\.tsx?$/,
                use: "ts-loader",
                exclude: /node_modules/,
            },
            {
                test: /\.(png|svg|jpg|jpeg|gif)$/i,
                type: "asset/resource",
            },
            {
                test: /\.(ogg)$/i,
                type: "asset/resource",
            },
        ],
    },
    mode: "development",
    devServer: {
        static: path.join(__dirname, "dist"),
        port: 8080,
    },
    experiments: {
        asyncWebAssembly: true,
    },
};

module.exports = config;
