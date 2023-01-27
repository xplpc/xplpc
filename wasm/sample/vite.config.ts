/// <reference types="vitest" />
import { defineConfig } from 'vite'

import vue from '@vitejs/plugin-vue'
import { resolve } from 'path'
import { fileURLToPath, URL } from "url"
import EnvironmentPlugin from "vite-plugin-environment"
import eslintPLugin from "vite-plugin-eslint"
import wasm from "vite-plugin-wasm";
import topLevelAwait from "vite-plugin-top-level-await";

const BASE_URL = (process.env.BASE_URL ? process.env.BASE_URL.replace(/\/$|$/, '/') : '/');

// https://vitejs.dev/config/
export default defineConfig({
    base: BASE_URL,
    build: {
        target: ['esnext']
    },
    resolve: {
        alias: {
            "@": fileURLToPath(new URL("./src", import.meta.url)),
            '@xplpc': resolve(__dirname, '../../'),
            '@bootstrap': resolve(__dirname, 'node_modules/bootstrap'),
        },
    },
    plugins: [
        vue(),
        eslintPLugin({
            exclude: ["../../build/**", "**/node_modules/**"]
        }),
        EnvironmentPlugin(
            {
                CURRENT_DATE_TIME:
                    new Date().toJSON().slice(0, 19).replace("T", " ") + " UTC",
            },
            {
                defineOn: "import.meta.env",
            }
        ),
        wasm(),
        topLevelAwait()
    ],
    server: {
        fs: {
            strict: false,
            allow: [".."],
        },
    },
    test: {
        globals: true,
        environment: 'jsdom',
        setupFiles: './src/setupTests.ts',
        coverage: {
            reporter: ['text', 'html'],
            exclude: [
                'node_modules/',
                'src/setupTests.ts',
            ],
        },
    }
})
