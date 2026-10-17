# Dota Item Merge

A Suika-style desktop game built in C17 with Raylib 5.5 and Box2D 3.1.1. Drop Dota-inspired items into a stash; matching items combine into the next tier. Work your way from Iron Branch to Divine Rapier before the stash overflows.

## Play

- Move the mouse, A/D, or left/right arrows to aim.
- Click inside the stash or press Space to drop. Deliveries have a short cooldown.
- P pauses; R starts a new run; Escape exits.
- Items that remain above the dashed line for two seconds end the run. New drops and newly merged items have a short grace period.
- Divine Rapiers are the final tier and do not merge further.

The game has Box2D circle physics with gravity, friction, rotation, sleeping, and continuous collisions, eleven tiers, merge effects, a next-item preview, keyboard controls, a resizable window, and a personal best saved as `dota-item-merge.best` in the working directory.

## Build and run

Install a C17-capable C compiler, a C++ compiler (required by Box2D's CMake configuration), and CMake 3.22+. CMake downloads the pinned Box2D 3.1.1 source with a SHA-256 check. It uses an installed Raylib 5.5 package or downloads the pinned Raylib source. The first build requires internet access.

### Linux (Debian / Ubuntu)

Install Raylib's desktop build dependencies:

```sh
sudo apt-get update
sudo apt-get install build-essential cmake libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/dota-item-merge
```

### macOS

Install the Xcode command line tools and CMake (`brew install cmake`), then run the same CMake and executable commands above.

### Windows

Install Visual Studio with Desktop development with C++ and CMake:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\dota-item-merge.exe
```

A working graphics driver and desktop display are required. No runtime asset downloads are needed.

## Test the simulation

On Linux/macOS, run the simulation tests through Make and CMake:

```sh
make test
```

Or use CMake without downloading Raylib or requiring a graphics context:

```sh
cmake -S . -B build-tests -DBUILD_GAME=OFF
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

Tests cover every merge tier, final-tier behavior, different-tier collisions, simultaneous contacts, drop limits and cooldown, floor settling, fast wall/floor impacts, rotation, overflow, invalid inputs, full capacity, a seeded collision stress run, and repeated world destruction/recreation. Test builds download Box2D but do not need Raylib or a display.

## Source

- `src/game.c`, `src/game.h`: Box2D integration and Raylib-independent game rules.
- `cmake/Box2D.cmake`: shared, pinned Box2D dependency for desktop, tests, and Android.
- `src/main.c`: Raylib rendering, input, procedural icons, and local best score.
- `tests/test_game.c`: simulation tests.

This is an unofficial fan prototype. The upgrade sequence is a game mechanic, not Dota 2's crafting recipes. Icons are original procedural drawings; no Valve artwork is bundled. Dota 2 and item names belong to Valve. Raylib is distributed under its own zlib license; [Box2D](https://github.com/erincatto/box2d/tree/v3.1.1) uses the MIT license.

## Validation

Debug and Release simulation tests pass, including Box2D's internal validation. AddressSanitizer, UndefinedBehaviorSanitizer, and leak detection pass. The desktop game links successfully, and a rendering harness verifies the portrait layout with Box2D running. The Android 0.2.1 debug APK builds for ARM64 and ARMv7; signatures, ZIP alignment, and native 16 KB alignment pass. On-device Box2D playtesting remains.

## Physics integration

Box2D advances at 120 fixed steps per second with four solver substeps. Coordinates convert at 50 pixels per meter; equal-density circle bodies provide mass proportional to their area. The stash uses a static floor and two side walls, with an open top. Rendering follows body positions and rotation.

Contact events are collected after each step, then matching pairs are merged after the event buffer has been read. Each item can join at most one pair per step. Both old bodies are destroyed and a larger circle is created inside the stash boundaries, retaining the scoring, merge effects, and overflow grace period.

A `Game` owns its world: call `game_init` once, `game_reset` for a new run, and `game_destroy` when finished. Do not copy a live `Game`; its shape user data refers to its item array. Use `game_spawn` for simulation fixtures rather than writing new items directly into that array.

## Android

An Android native build with immersive fullscreen, drag/release touch controls, on-screen pause/restart buttons, and a GitHub Actions APK build are included. See [Android instructions](android/README.md) for producing and installing a signed test APK. The signed Android 0.2.1 Box2D debug build has been verified for ARM64 and ARMv7, with a responsive portrait layout and immersive fullscreen; on-device fullscreen testing remains.
