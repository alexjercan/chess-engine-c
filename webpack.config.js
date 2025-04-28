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
            cFiles: ["src/main.c"],
            outputFileName: "main.wasm",
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
