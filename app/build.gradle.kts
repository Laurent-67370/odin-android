plugins {
    id("com.android.application")
}

// Odin's sources, fetched and patched by fetch-odin.sh (see README.md)
val odinSrc = rootProject.file("odin-src")
val glslang = System.getenv("GLSLANG") ?: "/usr/bin/glslangValidator"

android {
    namespace = "org.odinport.android"
    compileSdk = 35

    defaultConfig {
        applicationId = "org.odinport.android"
        minSdk = 26
        targetSdk = 35
        // Above the builds of Emberwake (its run numbers, up to ~50): Android
        // installs over only a version of a higher code
        versionCode = 1000 + (System.getenv("GITHUB_RUN_NUMBER") ?: "1").toInt()
        versionName = "0.1-" + (System.getenv("ODIN_COMMIT") ?: "dev").take(7)

        ndk { abiFilters += listOf("arm64-v8a") }
        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DANDROID_STL=c++_shared",
                    "-DODIN_BUNDLED_LIBS=ON",
                    "-DODIN_NATIVE=OFF",
                    "-DGLSLANG=$glslang",
                    "-DAWK=/usr/bin/awk",
                    "-DODIN_VERSION=android-" + (System.getenv("ODIN_COMMIT") ?: "dev").take(7),
                )
                targets += listOf("odin", "SDL3-shared")
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("${odinSrc}/CMakeLists.txt")
        }
    }

    // One key for every build, so that a new APK installs over the last one
    // (the debug key of each CI machine is new every time). The workflow
    // writes it from the secrets ODIN_KEYSTORE_B64 and ODIN_KEYSTORE_PASSWORD;
    // without them the APK is signed with the debug key, as before.
    val keystore = System.getenv("ODIN_KEYSTORE")?.let { file(it) }?.takeIf { it.isFile }
    val keystorePassword = System.getenv("ODIN_KEYSTORE_PASSWORD")
    signingConfigs {
        if (keystore != null && !keystorePassword.isNullOrEmpty()) {
            create("odin") {
                storeFile = keystore
                storeType = "pkcs12"
                storePassword = keystorePassword
                keyAlias = "odin"
                keyPassword = keystorePassword
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            signingConfig = signingConfigs.findByName("odin") ?: signingConfigs.getByName("debug")
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    // SDL's Java side, from the same SDL that is built in
    sourceSets["main"].java.srcDirs("${odinSrc}/3rdparty/SDL/android-project/app/src/main/java")
    sourceSets["main"].assets.srcDirs(layout.buildDirectory.dir("generated/odinAssets"))
    packaging { jniLibs { useLegacyPackaging = false } }
}

// Odin's data (game list, picture filters compiled to SPIR-V), unpacked by
// SetupActivity at the first start
val odinData by tasks.registering(Exec::class) {
    val out = layout.buildDirectory.dir("generated/odinAssets/odin").get().asFile
    inputs.dir(odinSrc.resolve("filters"))
    inputs.file(odinSrc.resolve("odin-games.json"))
    outputs.dir(out)
    environment("GLSLANG", glslang)
    commandLine("sh", rootProject.file("tools/build-data.sh").path, odinSrc.path, out.path)
}
tasks.named("preBuild") { dependsOn(odinData) }
