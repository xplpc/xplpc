<script lang="ts" setup>
import { useRouter } from 'vue-router';

const router = useRouter();
</script>

<template>
    <nav class="navbar navbar-light bg-light">
        <div class="container-fluid">
            <a class="navbar-brand" href="#" @click="router.push('/')">
                <img src="@/assets/images/logo.png" alt="Logo" width="120" class="d-inline-block align-text-top">
            </a>

            <button
type="button" data-bs-toggle="collapse" data-bs-target="#navbarNav" aria-controls="navbarNav"
                aria-expanded="false" aria-label="Toggle navigation" class="navbar-toggler">
                <span class="navbar-toggler-icon"></span>
            </button>

            <div id="navbarNav" class="collapse navbar-collapse">
                <ul class="navbar-nav">
                    <li class="nav-item">
                        <a class="nav-link" href="#" @click="router.push('/')">Home</a>
                    </li>
                    <li class="nav-item">
                        <a class="nav-link" href="#" @click="router.push('/about')">About</a>
                    </li>
                </ul>
            </div>
        </div>
    </nav>

    <router-view />
</template>
