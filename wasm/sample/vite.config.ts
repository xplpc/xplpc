import vue from '@vitejs/plugin-vue'
import { resolve } from 'path'
import { fileURLToPath, URL } from "url"
import { defineConfig } from 'vite'
import EnvironmentPlugin from "vite-plugin-environment"
import eslintPLugin from "vite-plugin-eslint"

const BASE_URL = process.env.BASE_URL || '';

// https://vitejs.dev/config/
export default defineConfig({
    base: `${BASE_URL}/`,
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
    ],
    server: {
        fs: {
            strict: false,
            allow: [".."],
        },
    },
})
