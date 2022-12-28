import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";

const BASE_URL = (import.meta.env.BASE_URL ? import.meta.env.BASE_URL.replace(/\/$|$/, '/') : '/');

const routes: Array<RouteRecordRaw> = [
    {
        path: "/",
        name: "main",
        component: () => import("../components/pages/MainPage.vue")
    },
    {
        path: "/about",
        name: "about",
        component: () => import("../components/pages/AboutPage.vue")
    }
]

const router = createRouter({
    history: createWebHistory(BASE_URL),
    routes,
})

export default router
