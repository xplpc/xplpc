// imports
import { createPinia } from "pinia";
import { createApp } from "vue";

// application
import App from "./App.vue";
const app = createApp(App);

// styles
import 'bootstrap'
import "./assets/css/app.scss"

// wasm
import WasmModule from "./plugin/wasm-module";
app.use(WasmModule);

// routes
import router from "./router";

// mount application
app.use(createPinia());
app.use(router);
app.mount("#app");
