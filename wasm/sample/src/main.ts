import { createPinia } from "pinia";
import { createApp } from "vue";

import App from "./App.vue";
const app = createApp(App);

import "bootstrap";
import "./assets/css/app.scss";

app.use(createPinia());

import WasmModule from "./plugin/wasm-module";
app.use(WasmModule);

import router from "./router";
app.use(router);

app.mount("#app");
