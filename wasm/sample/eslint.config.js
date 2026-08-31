import js from "@eslint/js";
import prettier from "@vue/eslint-config-prettier";
import {
    defineConfigWithVueTs,
    vueTsConfigs,
} from "@vue/eslint-config-typescript";
import vue from "eslint-plugin-vue";
import globals from "globals";

export default defineConfigWithVueTs(
    {
        ignores: [
            "dist/**",
            "coverage/**",
            "node_modules/**",
            "../../build/**",
        ],
    },
    js.configs.recommended,
    vue.configs["flat/recommended"],
    vueTsConfigs.recommended,
    prettier,
    {
        languageOptions: {
            globals: {
                ...globals.browser,
                ...globals.node,
            },
        },
    },
);
