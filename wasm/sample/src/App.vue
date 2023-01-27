<template>
    <nav class="navbar navbar-light bg-light">
        <div class="container-fluid">
            <router-link class="navbar-brand" :to="{ name: 'main' }">
                <img src="@/assets/images/logo.png" alt="Logo" width="120" class="d-inline-block align-text-top">
            </router-link>

            <button
type="button" data-bs-toggle="collapse" data-bs-target="#navbarNav" aria-controls="navbarNav"
                aria-expanded="false" aria-label="Toggle navigation" class="navbar-toggler">
                <span class="navbar-toggler-icon"></span>
            </button>

            <div id="navbarNav" class="collapse navbar-collapse">
                <ul class="navbar-nav">
                    <li class="nav-item">
                        <router-link class="nav-link" :to="{ name: 'main' }">
                            Home
                        </router-link>
                    </li>
                    <li class="nav-item">
                        <router-link class="nav-link" :to="{ name: 'about' }">
                            About
                        </router-link>
                    </li>
                </ul>
            </div>
        </div>
    </nav>

    <router-view />
</template>
