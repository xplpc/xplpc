/// <reference types="vitest/config" />
import { defineConfig } from 'vite'

import vue from '@vitejs/plugin-vue'
import { resolve } from 'path'
import { fileURLToPath, URL } from "url"
import EnvironmentPlugin from "vite-plugin-environment"
import eslintPLugin from "vite-plugin-eslint"

const BASE_URL = (process.env.BASE_URL ? process.env.BASE_URL.replace(/\/$|$/, '/') : '/');

const keepImportMetaUrlPlugin = {
    name: "keep-import-meta-url",
    enforce: "pre",
    transform(code, id, _options) {
        if (id.endsWith("xplpc.js")) {
            return code.replace(/\bimport\.meta\.url\b/g, `String(import.meta.url)`);
        }
    },
};

// https://vite.dev/config/
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
        ...(process.env.VITEST ? [keepImportMetaUrlPlugin] : [])
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
