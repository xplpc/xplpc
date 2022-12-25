<script lang="ts" setup>
import { XRemoteClient as client } from "@/xplpc/client/remote-client";
import { XParam } from "@/xplpc/message/param";
import { XRequest } from "@/xplpc/message/request";
import { ref } from "vue";
import { useWasmModuleStore } from "../../store/wasm-module";

const wm = useWasmModuleStore();
const fieldUsername = ref(null);
const fieldPassword = ref(null);
const fieldRemember = ref(false);

const defaultResponseMessage = "Use account paulo/123456 to success login";
const responseMessage = ref(defaultResponseMessage);

function onSubmit(e: Event): void {
    e.preventDefault();

    showResponse("Calling...");

    const request = new XRequest("sample.login",
        new XParam("username", fieldUsername.value),
        new XParam("password", fieldPassword.value),
        new XParam("remember", fieldRemember.value),
    );

    client.callAsync<string>(request).then((data) => {
        showResponse("Response: " + data);
    });
}

function showResponse(data: string | null): void {
    if (data) {
        responseMessage.value = data;
    } else {
        responseMessage.value = "ERROR!";
    }
}

function onReset(): void {
    fieldUsername.value = null;
    fieldPassword.value = null;
    fieldRemember.value = false;
    responseMessage.value = defaultResponseMessage;
}
</script>

<template>
    <div class="container">
        <div v-if="wm.isLoaded">
            <div class="text-left">
                <h4 id="pageTitle" class="my-5 text-center">Login</h4>

                <form @submit="onSubmit">
                    <div class="mb-3">
                        <label for="fieldUsername" class="form-label">Username</label>
                        <input id="fieldUsername" v-model="fieldUsername" type="text" class="form-control">
                    </div>

                    <div class="mb-3">
                        <label for="fieldPassword" class="form-label">Password</label>
                        <input id="fieldPassword" v-model="fieldPassword" type="password" class="form-control">
                    </div>

                    <div class="mb-3 form-check">
                        <input id="fieldRemember" v-model="fieldRemember" type="checkbox" class="form-check-input">
                        <label class="form-check-label" for="fieldRemember">Remember</label>
                    </div>

                    <p class="my-5 text-center">
                        <strong><small id="responseMessage" class="text-">{{ responseMessage }}</small></strong>
                    </p>

                    <div class="form-bt-group">
                        <button type="submit" class="btn btn-primary">Submit</button>
                        <button type="button" class="btn btn-primary" @click="onReset">Clear</button>
                    </div>
                </form>
            </div>
        </div>
        <div v-else>
            <div class="text-center">
                <p class="my-5">Loading...</p>
            </div>
        </div>
    </div>
</template>
