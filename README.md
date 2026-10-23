<!-- markdownlint-disable-file MD033 -->

# SpaceCadetPinball

## Summary

Reverse engineering of `3D Pinball for Windows - Space Cadet`, a game bundled with Windows.

## How to play

Place compiled executable into a folder containing original game resources (not included).\
Supports data files from Windows and Full Tilt versions of the game.

## Web version (this fork)

**Play it: <https://hanclinto.github.io/SpaceCadetPinball/>** (bring your own game files, see below)

This fork is the [Emscripten port by alula](https://github.com/alula/SpaceCadetPinball) brought up to upstream v2.1,
with play on phones and a hook for sharing high scores:

- **Upstream v2.1**: all the gameplay fixes, volume controls, key and controller remapping, translations.
- **Phones and tablets**: the left and right halves of the screen are the flippers; hold *Launch* to pull the
  plunger. Give the phone a sharp jolt left, right or up to nudge the table (iOS asks for motion access on the
  first tap). Held upright, the table fills the width and the score panel sits below it.
- **Smooth on any screen**: the game updates at a fixed rate (120 per second, as on the desktop) and draws once
  per browser frame, on 60 Hz and 120 Hz screens alike.
- **Settings persist** in the browser (IndexedDB).
- **Your own game files**: a build without them (like the one on GitHub Pages) asks for them once: the folder, the
  files, or a .zip, from Windows XP (`C:\Program Files\Windows NT\Pinball`) or Full Tilt! Pinball (the Space Cadet
  table's folder, `CADET.DAT` and its `SOUND` folder). They stay in the browser. Full Tilt's data gives the Full Tilt
  edition of the table: higher resolution art, multiball, three music tracks. The game's files belong to Microsoft
  and Maxis and aren't part of this repository or its web page.
- **Shared high scores** (optional): a page can set `Module.scoreboard` before the game's script loads. The game then
  keeps no local table; instead it calls
  - `onTableNeeded()` when it wants the top 5, which the page passes back with
    `Module.ccall('web_set_high_score', null, ['number', 'string', 'number'], [position, name, score])`
    (also any time later, e.g. when someone else sets a new record),
  - `submit({scores, players, cheated})` when a game ends (`scores` per player; `cheated` is true if any cheat was
    on, including ones that outlast the game they were typed in), and
  - `show()` for *Game > High Scores*.

  Without `Module.scoreboard` the game keeps its usual high score table in the browser.

### Building

To include the game's files in the build, put them (everything but the .exe) in `game_resources/`; leave it empty
for a page that asks for them. Then, with the [Emscripten SDK](https://emscripten.org) (tested with 3.1.74):

```sh
emcmake cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

or without installing it: `docker run --rm -v "$PWD":/src -w /src emscripten/emsdk:3.1.74 sh -c "emcmake cmake -B build
-DCMAKE_BUILD_TYPE=Release && cmake --build build"`. The page is `bin/SpaceCadetPinball.html`, next to its `.js`,
`.wasm` and `.data`; serve the folder over HTTP.

## Known source ports

| Platform           | Author          | URL                                                                                                        |
| ------------------ | --------------- | ---------------------------------------------------------------------------------------------------------- |
| PS Vita            | Axiom           | <https://github.com/suicvne/SpaceCadetPinball_Vita>                                                        |
| Emscripten         | alula           | <https://github.com/alula/SpaceCadetPinball> <br> Play online: <https://alula.github.io/SpaceCadetPinball> |
| Nintendo Switch    | averne          | <https://github.com/averne/SpaceCadetPinball-NX>                                                           |
| webOS TV           | mariotaku       | <https://github.com/webosbrew/SpaceCadetPinball>                                                           |
| Android (WIP)      | Iscle           | https://github.com/Iscle/SpaceCadetPinball                                                                 |
| Nintendo Wii       | MaikelChan      | https://github.com/MaikelChan/SpaceCadetPinball                                                            |
| Nintendo 3DS       | MaikelChan      | https://github.com/MaikelChan/SpaceCadetPinball/tree/3ds                                                   |
| Nintendo DS        | Headshotnoby    | https://github.com/headshot2017/3dpinball-nds                                                              |
| Nintendo Wii U     | IntriguingTiles | https://github.com/IntriguingTiles/SpaceCadetPinball-WiiU                                                  |
| PlayStation 2      | Headshotnoby    | https://github.com/headshot2017/3dpinball-ps2                                                              |
| Sega Dreamcast     | Headshotnoby    | https://github.com/headshot2017/3dpinball-dc                                                               |
| MorphOS            | BeWorld         | https://www.morphos-storage.net/?id=1688897                                                                |
| AmigaOS 4          | rjd324          | http://aminet.net/package/game/actio/spacecadetpinball-aos4                                                |
| Android (WIP)      | fexed           | https://github.com/fexed/Pinball-on-Android                                                                |

Platforms covered by this project: desktop Windows, Linux and macOS.

<br>
<br>
<br>
<br>
<br>
<br>

## Source

* `pinball.exe` from `Windows XP` (SHA-1 `2A5B525E0F631BB6107639E2A69DF15986FB0D05`) and its public PDB
* `CADET.EXE` 32bit version from `Full Tilt! Pinball` (SHA-1 `3F7B5699074B83FD713657CD94671F2156DBEDC4`)

## Tools used

`Ghidra`, `Ida`, `Visual Studio`

## What was done

* All structures were populated, globals and locals named.
* All subs were decompiled, C pseudo code was converted to compilable C++. Loose (namespace?) subs were assigned to classes.

## Compiling

Project uses `C++11` and depends on `SDL2` libs.

### On Windows

Download and unpack devel packages for `SDL2` and `SDL2_mixer`.\
Set paths to them in `CMakeLists.txt`, see suggested placement in `/Libs`.\
Compile with Visual Studio; tested with 2019.

### On Linux

Install devel packages for `SDL2` and `SDL2_mixer`.\
Compile with CMake; tested with GCC 10, Clang 11.\
To cross-compile for Windows, install a 64-bit version of mingw and its `SDL2` and `SDL2_mixer` distributions, then use the `mingwcc.cmake` toolchain.

[![Packaging status](https://repology.org/badge/tiny-repos/spacecadetpinball.svg)](https://repology.org/project/spacecadetpinball/versions) 

Some distributions provide a package in their repository. You can use those for easier dependency management and updates.

This project is available as Flatpak on [Flathub](https://flathub.org/apps/details/com.github.k4zmu2a.spacecadetpinball).

### On macOS

Install XCode (or at least Xcode Command Line Tools with `xcode-select --install`) and CMake.

**HomeBrew**

You can easily install the build artifact by using `brew`.

```sh
brew tap draftbrew/tap
brew install --no-quarantine space-cadet-pinball
```

Be aware that the flag `--no-quarantime` will disable macOS's Gatekeeper during installation.

**Manual compilation:**

* **Homebrew**: Install the `SDL2`, `SDL2_mixer` homebrew packages.
* **MacPorts**: Install the `libSDL2`, `libSDL2_mixer` macports packages.

Compile with CMake. Ensure that `CMAKE_OSX_ARCHITECTURES` variable is set for either `x86_64` Apple Intel or `arm64` for Apple Silicon.

Tested with: macOS Big Sur (Intel) with Xcode 13 & macOS Montery Beta (Apple Silicon) with Xcode 13.

**Automated compilation:**

Run the `build-mac-app.sh` script from the root of the repository. The app will be available in a DMG file named `SpaceCadetPinball-<version>-mac.dmg`.

Tested with: macOS Ventura (Apple Silicon) with Xcode Command Line Tools 14 & macOS Big Sur on GitHub Runner (Intel) with XCode 13.

## Plans

* ~~Decompile original game~~
* ~~Resizable window, scaled graphics~~
* ~~Loader for high-res sprites from CADET.DAT~~
* ~~Cross-platform port using SDL2, SDL2_mixer, ImGui~~
* Full Tilt Cadet features
* Localization support
* Maybe: Support for the other two tables - Dragon and Pirate
* Maybe: Game data editor

## On 64-bit bug that killed the game

I did not find it, decompiled game worked in x64 mode on the first try.\
It was either lost in decompilation or introduced in x64 port/not present in x86 build.\
Based on public description of the bug (no ball collision), I guess that the bug was in `TEdgeManager::TestGridBox`
