plugins {
    id("com.android.application")
    id("dev.flutter.flutter-gradle-plugin")
}

android {
    namespace = "com.xplpc.runner.flutter"
    compileSdk = 37
    ndkVersion = flutter.ndkVersion

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    defaultConfig {
        applicationId = "com.xplpc.runner.flutter"
        minSdk = flutter.minSdkVersion
        targetSdk = 37
        versionCode = flutter.versionCode
        versionName = flutter.versionName

        ndk {
            debugSymbolLevel = "full"
        }
    }

    buildTypes {
        release {
            // The release variant is signed with the debug keys so the example runs, and a real application replaces this.
            signingConfig = signingConfigs.getByName("debug")
        }
    }
}

kotlin {
    compilerOptions {
        jvmTarget = org.jetbrains.kotlin.gradle.dsl.JvmTarget.JVM_17
    }
}

flutter {
    source = "../.."
}

// An aar file carries no dependency metadata, so the libraries the kotlin layer uses are named beside it.
dependencies {
    implementation(files("${rootProject.projectDir}/../../../../build/kotlin-aar-flutter/library-release.aar"))
    implementation("com.google.code.gson:gson:2.15.0")
    implementation("org.jetbrains.kotlin:kotlin-reflect")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-core:1.11.0")
}
