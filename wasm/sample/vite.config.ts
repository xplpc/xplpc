/// <reference types="vitest/config" />
import { defineConfig } from 'vite'

import vue from '@vitejs/plugin-vue'
import { resolve } from 'path'
import { fileURLToPath, URL } from "url"
import EnvironmentPlugin from "vite-plugin-environment"
import eslintPLugin from "vite-plugin-eslint"

const BASE_URL = (process.env.BASE_URL ? process.env.BASE_URL.replace(/\/$|$/, '/') : '/');

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
        {
            name: "keep-import-meta-url",
            enforce: "pre",
            transform(code, id, _options) {
                // prevent NormalizeURLPlugin from replacing import.meta.url with self.location
                // https://github.com/vitest-dev/vitest/blob/d8304bb4fbe16285d014f63aa71ef9969865c691/packages/vitest/src/node/plugins/normalizeURL.ts#L11
                // since it breaks `new URL(..., import.meta.url)` used by emscripten EXPORT_ES6 output
                // https://github.com/emscripten-core/emscripten/blob/228af1a7de1672b582e1448d4573c20c5d2a5b5a/src/shell.js#L242
                if (id.endsWith("xplpc.js")) {
                    return code.replace(/\bimport\.meta\.url\b/g, `String(import.meta.url)`);
                }
            },
        }
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
