import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";

const BASE_URL = process.env.BASE_URL || '';

const routes: Array<RouteRecordRaw> = [
    {
        path: "/",
        name: "Main",
        component: () => import("../components/pages/MainPage.vue")
    },
    {
        path: "/about",
        name: "About",
        component: () => import("../components/pages/AboutPage.vue")
    }
]

const router = createRouter({
    history: createWebHistory(BASE_URL),
    routes,
})

export default router
