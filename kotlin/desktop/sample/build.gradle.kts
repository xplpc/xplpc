import org.jetbrains.compose.desktop.application.dsl.TargetFormat
import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins {
    kotlin("jvm")
    id("org.jetbrains.compose")
    id("org.jetbrains.kotlin.plugin.compose")
}

group = "com.xplpc.runner.compose"
version = "1.0.0"

repositories {
    mavenCentral()
    google()
}

dependencies {
    implementation(compose.desktop.currentOs)
    implementation("org.jetbrains.compose.material:material-icons-core:1.7.3")

    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-core:1.11.0")
    implementation("com.google.code.gson:gson:2.15.0")
    implementation("org.jetbrains.kotlin:kotlin-reflect")

    implementation(files("../../../build/kotlin-jar-desktop/xplpc.jar"))
}

compose.desktop {
    application {
        mainClass = "MainKt"

        nativeDistributions {
            targetFormats(TargetFormat.Dmg, TargetFormat.Msi, TargetFormat.Deb)
            packageName = "runner"
            packageVersion = "1.0.0"
        }
    }
}

java {
    sourceCompatibility = JavaVersion.VERSION_17
    targetCompatibility = JavaVersion.VERSION_17
}

kotlin {
    compilerOptions {
        jvmTarget = JvmTarget.JVM_17
        freeCompilerArgs.add("-Xjdk-release=17")
    }
}
