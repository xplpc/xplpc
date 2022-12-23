import { createRouter, createWebHistory, RouteRecordRaw } from "vue-router"

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
    history: createWebHistory(),
    routes,
})

export default router
