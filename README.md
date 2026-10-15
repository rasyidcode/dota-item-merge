# Dota Item Merge

A Suika-style desktop game built in C99 with Raylib 5.5. Drop Dota-inspired items into a stash; matching items combine into the next tier. Work your way from Iron Branch to Divine Rapier before the stash overflows.

## Play

- Move the mouse, A/D, or left/right arrows to aim.
- Click inside the stash or press Space to drop. Deliveries have a short cooldown.
- P pauses; R starts a new run; Escape exits.
- Items that remain above the dashed line for two seconds end the run. New drops and newly merged items have a short grace period.
- Divine Rapiers are the final tier and do not merge further.

The game has circle physics with mass-weighted collisions, eleven tiers, merge effects, a next-item preview, keyboard controls, a resizable window, and a personal best saved as `dota-item-merge.best` in the working directory.

## Build and run

Install a C compiler and CMake 3.20+. CMake uses an installed Raylib 5.5 package or downloads the pinned Raylib 5.5 source on the first build.

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

On Linux/macOS, the physics tests need only a C compiler and Make:

```sh
make test
```

Or use CMake without downloading Raylib or requiring a graphics context:

```sh
cmake -S . -B build-tests -DBUILD_GAME=OFF
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

Tests cover every merge tier, final-tier behavior, different-tier collisions, drop limits and cooldown, floor settling, overflow, and a seeded collision stress run.

## Source

- `src/game.c`, `src/game.h`: Raylib-independent fixed-step physics and game rules.
- `src/main.c`: Raylib rendering, input, procedural icons, and local best score.
- `tests/test_game.c`: simulation tests.

This is an unofficial fan prototype. The upgrade sequence is a game mechanic, not Dota 2's crafting recipes. Icons are original procedural drawings; no Valve artwork is bundled. Dota 2 and item names belong to Valve. Raylib is distributed under its own zlib license.

## Validation

The simulation tests pass with strict compiler warnings and AddressSanitizer/UndefinedBehaviorSanitizer (leak detection disabled because of the sandbox). Rendering source passes C99 syntax checks against the official Raylib 5.5 header. A linked graphical build and visual playtest remain to be run on a desktop with Raylib build dependencies; the development sandbox had no display or Raylib installation and its download proxy was unreachable.

## Android

An Android native build, drag/release touch controls, on-screen pause/restart buttons, and a GitHub Actions APK build are included. See [Android instructions](android/README.md) for producing and installing a signed test APK. Android builds have not yet been verified in the development sandbox.
