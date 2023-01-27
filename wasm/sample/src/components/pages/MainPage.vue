<script lang="ts" setup>
import { XLocalClient } from "@/xplpc/client/local-client";
import { XRemoteClient } from "@/xplpc/client/remote-client";
import { XParam } from "@/xplpc/message/param";
import { XRequest } from "@/xplpc/message/request";
import { ref } from "vue";
import { useWasmModuleStore } from "../../store/wasm-module";

const wm = useWasmModuleStore();
const fieldUsername = ref(null);
const fieldPassword = ref(null);
const fieldRemember = ref(false);

const defaultResponseMessageForForm = "Use account paulo/123456 to success login";
const responseMessageForForm = ref(defaultResponseMessageForForm);

const defaultResponseMessageForBattery = "Press Button To Execute";
const responseMessageForBattery = ref(defaultResponseMessageForBattery);

async function onSubmitForm(e: Event) {
    e.preventDefault();

    showResponseForForm("Calling...");

    const request = new XRequest(
        "sample.login",
        new XParam("username", fieldUsername.value),
        new XParam("password", fieldPassword.value),
        new XParam("remember", fieldRemember.value),
    );

    let response = await XRemoteClient.call<string>(request);

    if (response) {
        showResponseForForm("Response: " + response);
    } else {
        showResponseForForm("Response: ERROR!");
    }
}

async function onSubmitBatteryLevel(e: Event) {
    e.preventDefault();

    showResponseForBattery("Calling...");

    const request = new XRequest(
        "platform.battery.level",
        new XParam("suffix", "%"),
    );

    let response = await XLocalClient.call<string>(request);

    if (response) {
        showResponseForBattery("Response: " + response);
    } else {
        showResponseForBattery("Response: ERROR!");
    }
}

function showResponseForForm(data: string | undefined): void {
    if (data) {
        responseMessageForForm.value = data;
    } else {
        responseMessageForForm.value = "ERROR!";
    }
}

function showResponseForBattery(data: string | undefined): void {
    if (data) {
        responseMessageForBattery.value = data;
    } else {
        responseMessageForBattery.value = "ERROR!";
    }
}

function onReset(): void {
    fieldUsername.value = null;
    fieldPassword.value = null;
    fieldRemember.value = false;

    responseMessageForForm.value = defaultResponseMessageForForm;
    responseMessageForBattery.value = defaultResponseMessageForBattery;
}
</script>

<template>
    <div class="container">
        <div v-if="wm.isLoaded">
            <div class="text-left">
                <!-- BATTERY LEVEL -->
                <h4 id="pageTitle" class="my-5 text-center">BATTERY LEVEL</h4>

                <p class="my-5 text-center">
                    <small id="responseMessageForBattery" class="text-center">{{
                        responseMessageForBattery
                    }}</small>
                </p>

                <div class="form-bt-group text-center mb-5">
                    <button type="button" class="btn btn-primary" @click="onSubmitBatteryLevel">Submit</button>
                </div>

                <!-- SEPARATOR -->

                <hr>

                <!-- LOGIN -->

                <h4 id="pageTitle" class="my-5 text-center">LOGIN</h4>

                <form @submit="onSubmitForm">
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
                        <small id="responseMessageForForm" class="text-center">{{
                            responseMessageForForm
                        }}</small>
                    </p>

                    <div class="form-bt-group text-center mb-5">
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
