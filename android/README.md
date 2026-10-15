# Android test build

This folder packages the C/Raylib game as a native Android app, with no Java game layer. It targets Android 6.0+ ARM phones (32-bit and 64-bit) and uses landscape orientation. The APK needs no network or storage permissions.

## Phone controls

- Touch inside the stash and drag horizontally to aim.
- Lift your finger to drop one item. There is a short delivery cooldown.
- Tap PAUSE / RESUME or NEW RUN in the left panel.
- Personal best is saved to Android's private app storage after scoring.

## Get an APK from GitHub Actions

Once the project files, including `.github/workflows/android.yml`, are pushed to the repository:

1. Open the repository's **Actions** tab and select **Android APK**.
2. Open the latest successful run (or choose **Run workflow**).
3. Download the `dota-item-merge-android` artifact and extract its ZIP.
4. Transfer `app-debug.apk` to your phone and open it. Allow your browser/file manager to install this app if Android asks.

The workflow uses JDK 17, Gradle 8.9, Android Gradle Plugin 8.7.3, Android SDK 35, NDK 28.0.13004108, and CMake 3.22.1. It builds and signs a debug APK for local testing. This is not a Play Store release. Clean GitHub runners create fresh debug signing keys; uninstall an older test build if Android reports an incompatible update (uninstalling clears its saved best).

## Build on your computer

Install Android Studio, JDK 17, and Gradle 8.9. In Android Studio's SDK Manager install Android SDK Platform 35, Build Tools 35.0.0, NDK 28.0.13004108, and CMake 3.22.1 (enable **Show Package Details** to select versions). Set `ANDROID_HOME` to your SDK directory, or put `sdk.dir=/absolute/path/to/Android/Sdk` in `android/local.properties`.

From the repository root:

```sh
cd android
gradle assembleDebug
```

On Windows use `gradle.bat assembleDebug`. Use the installed Gradle 8.9 distribution; a Gradle wrapper is not bundled. The first build downloads Raylib 5.5 and Gradle dependencies and needs internet access. Accept SDK licenses with `sdkmanager --licenses` if prompted by the build.

The signed test APK will be at:

```
android/app/build/outputs/apk/debug/app-debug.apk
```

If ADB and USB debugging are configured:

```sh
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

## Current validation status

The shared physics tests pass, and the desktop rendering source passes strict syntax checks against Raylib 5.5. The Android configuration and touch integration have been reviewed against Raylib's Android backend, but have **not** been compiled or tested on a phone: the development sandbox has no SDK/NDK/Gradle and cannot reach its download proxy. No APK is included in the source ZIP. A successful Android build and on-device test are still needed.
