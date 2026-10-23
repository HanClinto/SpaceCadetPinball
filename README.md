<!-- markdownlint-disable-file MD033 -->

# SpaceCadetPinball

## This fork: the web version, playable on phones

**Play it: <https://hanclinto.github.io/SpaceCadetPinball/>**, on a computer or a phone.

This fork takes [alula's Emscripten port](https://github.com/alula/SpaceCadetPinball) (2021), brings it up to the
decompilation's upstream v2.1, and makes it a game you can play on a phone. It was made for a family game site,
where everyone shares one high score table, and the hook for that is part of the fork, usable by any page.

### What this fork changes

**Brought up to upstream v2.1.** alula's port was built on the decompilation as it stood in September 2021; upstream
([k4zmu2a/SpaceCadetPinball](https://github.com/k4zmu2a/SpaceCadetPinball)) has 190 commits since. They're merged
in: gameplay fixes (mission scores, wormhole lights, flipper animation, hard-hit detection, plunger), volume
controls, stereo sound, key and game controller remapping, translations, and Full Tilt! Pinball support. alula's web
changes were re-applied on top:

- **Music** through TinySoundFont (SDL_mixer has no MIDI synthesizer in the browser), behind upstream's new
  multi-track music code, so Full Tilt's three tracks play too. Desktop builds keep SDL_mixer, as upstream.
- **A main loop for the browser.** The desktop game sleeps between updates; a page can't. Each browser frame
  (requestAnimationFrame) runs as many fixed-length updates as the time since the last one calls for, 120 per
  second as on the desktop, and draws once. The same physics on 60 Hz and 120 Hz screens; alula's version ran one
  update per 60 Hz timer tick, which made it choppy.

**Phones and tablets.**

- **Touch**: the left and right halves of the screen are the flippers; hold **Launch** (bottom right) to pull the
  plunger and let go to launch; after a game it becomes **New game**. The bar also has ☰, the menu (new game, pause,
  sound, music, the game's own menu bar for its options), ⛶ full screen and 🏆 the high scores.
- **Shake to nudge**: a sharp jolt of the device left, right or up bumps the table that way, as the nudge keys do
  (too many and it tilts, as in the original). The edge of the screen it was bumped from lights up, and Android
  phones buzz. It works wherever the browser gets motion readings (phones, tablets, laptops with a motion sensor);
  elsewhere the option is grayed out. The menu sets how hard a jolt it takes (low, medium, high sensitivity) or
  turns it off. iOS asks for motion access first.
- **Held upright**, the table fills the width and the score panel sits below it, its logo half and score half side
  by side; the score panel's text boxes follow. On a computer, or a phone held sideways, the layout is the original.
- **Sharp on high-density screens**: the canvas is drawn at the screen's full resolution, with the game's menus and
  dialogs scaled to stay readable.

**Two versions of the table.** XP Space Cadet (Windows XP's) and Full Tilt Space Cadet (the original Full Tilt!
Pinball edition: higher resolution art, multiball, jackpots, three music tracks). The page's address picks one
(`?table=fulltilt`, XP's otherwise) and the menu switches when both are there. The game now looks for the chosen
version's data in every folder before falling back to the other, so built-in XP files don't hide Full Tilt files
a player added.

**Game files.** They belong to Microsoft and Maxis, and none are in this repository's source.

- The GitHub Pages version includes the Windows XP files that alula's web build has published since 2021: the
  workflow (`.github/workflows/pages.yml`) takes them from this fork's `gh-pages` branch, which is that build, at
  a fixed commit.
- Players can add their own copy with **Add game files** in the menu, for instance Full Tilt! Pinball's Space Cadet
  folder (`CADET.DAT` and its `SOUND` folder): the folder, its files, or a .zip. Added files stay in the browser.
- A build without any game files asks for them before it starts.

**Settings persist** in the browser (IndexedDB): the desktop game writes them when it exits, which a page never
does, so the page saves them every few seconds and whenever it's hidden or closed.

**Shared high scores (optional).** A page can set `Module.scoreboard` before the game's script loads. The game then
keeps no local table and never asks for a name; instead it calls:

- `onTableNeeded()` when it wants the top 5; the page passes them back (then or any time later, e.g. when someone
  else sets a record) with `Module.ccall('web_set_high_score', null, ['number', 'string', 'number'], [position, name,
  score])`. The game uses them for its "High Score 1-5" messages and to say a game made the table.
- `submit({scores, players, cheated})` when a game ends: the score per player, and whether any cheat was on,
  including the ones that outlast the game they were typed in (easy mode, unlimited balls).
- `show()` for *Game > High Scores*.

`Module.ccall('web_game_state', 'number', [], [])` says whether a game is being played, is over or is the demo,
whether it's paused, and which version of the table is loaded (+8 for Full Tilt), so a page can keep a table per
version. Without `Module.scoreboard` the game keeps its usual high score table in the browser.

A ready-made one is in [`Platform/Web/scoreboard`](Platform/Web/scoreboard): `scoreboard.js` (the dialog after each
game, with personal bests, all-time and your own games) and a sample service with nothing to install,
`python3 Platform/Web/scoreboard/server.py bin`. The family site this fork was made for uses the same `scoreboard.js`.

**For page authors**, `SpaceCadetPinball/web.cpp` lists what a page can call (resize, input by game action, game
state, high scores, saving settings, the menu bar), and `emscripten_shell.html` is the page that uses them.

### Building

To include the game's files in the build, put them (everything but the .exe) in `game_resources/`: Windows XP's at
the top, and for Full Tilt, `CADET.DAT` with its `SOUND` folder. Leave it empty for a page that asks for them. Then,
with the [Emscripten SDK](https://emscripten.org) (tested with 3.1.74):

```sh
emcmake cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

or without installing it: `docker run --rm -v "$PWD":/src -w /src emscripten/emsdk:3.1.74 sh -c "emcmake cmake -B build
-DCMAKE_BUILD_TYPE=Release && cmake --build build"`. The page is `bin/SpaceCadetPinball.html`, next to its `.js`,
`.wasm` and `.data`; serve the folder over HTTP. `Platform/Web/unpack_data.py` unpacks another web build's `.data`
into a folder, which is how the Pages workflow gets its files. Desktop builds are unchanged from upstream.

---

*The rest is upstream's README, about the decompilation itself.*


## Summary

Reverse engineering of `3D Pinball for Windows - Space Cadet`, a game bundled with Windows.

## How to play

Place compiled executable into a folder containing original game resources (not included).\
Supports data files from Windows and Full Tilt versions of the game.

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
