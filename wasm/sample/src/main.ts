import "bootstrap";
import { createPinia } from "pinia";
import { createApp } from "vue";

import App from "./App.vue";
import "./assets/css/app.scss";
import WasmModule from "./plugin/wasm-module";
import router from "./router";

const app = createApp(App);

app.use(createPinia());
app.use(WasmModule);
app.use(router);

app.mount("#app");
