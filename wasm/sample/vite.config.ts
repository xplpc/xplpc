/// <reference types="vitest/config" />
import { defineConfig } from "vite";

import vue from "@vitejs/plugin-vue";
import { fileURLToPath, URL } from "url";
import checker from "vite-plugin-checker";
import EnvironmentPlugin from "vite-plugin-environment";

const BASE_URL = process.env.BASE_URL
    ? process.env.BASE_URL.replace(/\/$|$/, "/")
    : "/";

const keepImportMetaUrlPlugin = {
    name: "keep-import-meta-url",
    enforce: "pre" as const,
    transform(code: string, id: string) {
        if (id.endsWith("xplpc.js")) {
            return code.replace(
                /\bimport\.meta\.url\b/g,
                `String(import.meta.url)`,
            );
        }
    },
};

// The checker spawns its own type and lint processes, which is wasted work while running the suite.
const checkerPlugin = checker({
    eslint: {
        useFlatConfig: true,
        lintCommand: 'eslint "./src/**/*.{ts,vue}" "./tests/**/*.ts"',
    },
    vueTsc: true,
});

export default defineConfig({
    base: BASE_URL,
    build: {
        target: ["esnext"],
    },
    css: {
        preprocessorOptions: {
            scss: {
                // Bootstrap still ships @import and the legacy if(), so the warnings come from the dependency and not from this project.
                silenceDeprecations: [
                    "import",
                    "if-function",
                    "global-builtin",
                    "color-functions",
                ],
            },
        },
    },
    resolve: {
        alias: {
            "@": fileURLToPath(new URL("./src", import.meta.url)),
            "@xplpc": fileURLToPath(new URL("../../", import.meta.url)),
            "@bootstrap": fileURLToPath(
                new URL("./node_modules/bootstrap", import.meta.url),
            ),
        },
    },
    plugins: [
        vue(),
        EnvironmentPlugin(
            {
                CURRENT_DATE_TIME:
                    new Date().toJSON().slice(0, 19).replace("T", " ") + " UTC",
            },
            {
                defineOn: "import.meta.env",
            },
        ),
        ...(process.env.VITEST ? [keepImportMetaUrlPlugin] : [checkerPlugin]),
    ],
    server: {
        host: true,
        port: 3000,
        fs: {
            strict: false,
            allow: [".."],
        },
    },
    test: {
        globals: true,
        environment: "jsdom",
        setupFiles: "./src/setupTests.ts",
        coverage: {
            reporter: ["text", "html"],
            exclude: ["node_modules/", "src/setupTests.ts"],
        },
    },
});
