<script lang="ts" setup>
import { useWasmModuleStore } from "../../store/wasm-module";

const wm = useWasmModuleStore();
const currentDateTime = import.meta.env.CURRENT_DATE_TIME;
</script>

<template>
    <div class="container">
        <h4 class="my-5 text-center">About</h4>

        <div class="text-center">
            <div v-if="wm.isLoaded" class="q-mt-lg">
                <p>Generated at: {{ currentDateTime }}</p>
            </div>
            <div v-else class="q-mt-lg">
                <p>Loading...</p>
            </div>

            <br />

            <p>
                This is an open-source software made by:
                <br />
                <a href="https://github.com/paulocoutinhox" target="_blank">Paulo Coutinho</a>.
            </p>

            <br />

            <p>
                You can see the source code here:
                <br />
                <a href="https://github.com/xplpc/xplpc" target="_blank">
                    https://github.com/xplpc/xplpc
                </a>
            </p>

            <br />

            <hr />

            <br />

            <router-link class="btn btn-primary" :to="{ name: 'main' }">
                Back to Home
            </router-link>
        </div>
    </div>
</template>

<style scoped>
/* ignore */
</style>
