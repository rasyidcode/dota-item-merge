# Android test build

This folder packages the C/Raylib/Box2D game as a native Android app, with a small Java NativeActivity host for fullscreen system-bar control. It targets Android 6.0+ ARM phones (32-bit and 64-bit) and uses portrait orientation with a responsive phone layout and immersive fullscreen. All eleven original Dota 2 item icons are bundled inside the APK for offline play. The APK needs no network or storage permissions. See [asset attribution](../assets/README.md).

## Phone controls

- Touch inside the stash and drag horizontally to aim.
- Lift your finger to drop one item. There is a short delivery cooldown.
- Tap PAUSE / RESUME or NEW RUN below the stash.
- Press Back twice within two seconds to exit; the first press shows a message.
- Personal best is saved to Android's private app storage after scoring.
- Android's status and navigation bars stay hidden during play. Swipe from an edge to reveal them temporarily; they hide again automatically.

## Back to exit

Press Back once to show **Press back again to exit**. Press Back again within two seconds to close the game normally. After the timeout, Back starts a new confirmation window. Sending the app to the background clears the pending confirmation and message.

Android 13+ uses the platform `OnBackInvokedCallback` for system buttons and committed Back gestures. Older Android versions send Back through Raylib's native input queue; the game forwards completed, uncanceled Back releases to the same handler on the UI thread. Key-down and repeat events do not trigger confirmation. The native input hook preserves normal Raylib handling for touch and other keys. NativeActivity shutdown lets the game save its personal best and release the Box2D world.

The confirmation window is covered by five JVM unit tests, run with `gradle testDebugUnitTest` from this folder.

## Fullscreen behavior

`GameActivity` extends Android's `NativeActivity`; all gameplay and rendering remain native. It hides system bars at launch, after resume, and when window focus returns. Android 11+ uses `WindowInsetsController` with transient bars revealed by swipe; Android 6–10 uses sticky immersive flags. Layout stays behind transient bars so revealing navigation does not resize the game. This follows [Android's immersive-mode guidance](https://developer.android.com/develop/ui/views/layout/immersive) and adds no permissions or AndroidX dependency.

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

On Windows use `gradle.bat assembleDebug`. Use the installed Gradle 8.9 distribution; a Gradle wrapper is not bundled. The first build downloads Raylib 5.5, Box2D 3.1.1, and Gradle dependencies and needs internet access. Accept SDK licenses with `sdkmanager --licenses` if prompted by the build.

The signed test APK will be at:

```
android/app/build/outputs/apk/debug/app-debug.apk
```

If ADB and USB debugging are configured:

```sh
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

## Current validation status

Version 0.3.0 adds bundled Dota 2 item artwork, retains Back-to-exit confirmation, immersive fullscreen, and Box2D 3.1.1, and builds successfully as a signed debug APK for ARM64 and ARMv7 using the pinned toolchain. Physics tests pass in Debug and Release, including simultaneous merges, fast impacts, rotation, and repeated world recreation. AddressSanitizer, UndefinedBehaviorSanitizer, and leak detection pass. The renderer, Java host, and native input bridge compile. The existing five confirmation-window tests pass. Android lint completes with zero errors and five existing warnings. APK signatures, ZIP alignment, and 16 KB native-library alignment are verified.

All eleven Dota item icons were loaded and inspected in the renderer, with portrait layouts at 16:9 and 20:9 phone ratios, the pause screen, and Box2D-driven rotation. Gradle packages the shared `assets/` directory, and Raylib loads `items/*.png` through Android's asset manager. PNG decoding is explicitly enabled. The packaged images match the originals byte for byte. Android uses the native display resolution, avoiding extra Raylib letterboxing. Scores and the next delivery appear above the stash, with a compact upgrade path and large pause/restart controls below. Desktop retains its landscape layout. On-device checks remain for artwork, Back buttons and gestures, first-press messaging, timeout expiry, background reset, and clean exit while immersive fullscreen is active.
