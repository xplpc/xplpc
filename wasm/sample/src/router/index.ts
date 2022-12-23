import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router";

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
    history: createWebHistory(`${import.meta.env.BASE_UR}/`),
    routes,
})

export default router
