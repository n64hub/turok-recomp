# Turok: Recompiled

> **Made with AI.** Almost all of the code, documentation and tests in this repo were written by AI agents
> (Claude), directed and play-tested by a person. None of it is offered to the upstream projects it builds on, which
> don't take AI-written code.

A native PC port of the N64 *Turok: Dinosaur Hunter* (USA), made by static recompilation
([N64Recomp](https://github.com/N64Recomp/N64Recomp) + [N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime) +
[RT64](https://github.com/rt64/rt64), with [RecompFrontend](https://github.com/N64Recomp/RecompFrontend)'s launcher
and menus): the game's own MIPS code translated to C and run natively, with RT64 drawing it. It is not an emulator. The
functions are found in the ROM's code, with no decompilation, and what the Turok games share (the function finder, the
Controller Pak, libultra the runtime lacks) is in [turok-engine](https://github.com/n64hub/turok-engine), this repo's
`lib/turok-engine`.

The repo holds no ROM, no asset from the game and no data extracted from it: you supply your own ROM, and nothing in
git or CI is made from one except the symbol names, addresses and sizes in `syms/` and where the code is in the ROM
(`syms/layout.toml`).

**Status:** released for Windows and Linux (`releases/`). The desktop build plays from boot through the intro and the
title menu into the first level, with sound, and keyboard and gamepad input. The game hasn't been played past the first
level's start yet. The same game plays in a browser through WebGPU, at
[n64hub.com/turok-dinosaur-hunter](https://n64hub.com/turok-dinosaur-hunter/) (`docs/web.md`).

## The ROM

*Turok: Dinosaur Hunter* (USA, revision 1.0), as a big-endian `.z64` of 8 MiB with SHA-1
`40fb0250c095740031278fb1f82b9937a3895e01`. Other regions and revisions are not supported.

The game keeps its main program packed (RNC method 1). The build unpacks it from your ROM into
`roms/turok1.us.recomp.z64` (`tools/unpack_main.py`) for the recompiler; the game itself still unpacks it from the
ROM you load, as on the console.

## Building

You need that ROM at `roms/turok1.us.z64` and Python 3. On Windows: LLVM's clang-cl, CMake 3.31 and Ninja, and a
clang with a MIPS backend plus ld.lld for `patches/` (WSL's will do). On Linux: `clang lld libsdl2-dev libgtk-3-dev
libfreetype-dev`, CMake 3.31 and Ninja.

From the repo's root in a bash shell:

```sh
git -c core.longpaths=true submodule update --init --recursive   # paths past Windows' 260 characters
bash tools/recompile.sh -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl   # the game's code as C
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl \
      -DPATCHES_C_COMPILER=<MIPS clang> -DPATCHES_LD=<ld.lld>
ninja -C build TurokRecompiled
```

On Linux, the same with `clang` and `clang++`. `tools/recompile.sh` builds N64Recomp from the submodule the first
time, unpacks the main program, then translates the game. Run `build/TurokRecompiled` from the repo's root, pick the
ROM with Load ROM, then Start Game.

`syms/` is written from the unpacked ROM by `python3 lib/turok-engine/tools/gen_syms.py` (it needs
`pip install spimdisasm`); run it again only when the engine's finder or `syms/names.toml` changes.

## Releases

Each release is built where the ROM is, by `tools/release.sh` (its header says how), from the merge commit of a pull
request titled "Release v<version>" that bumps `version_string` in `src/main/main.cpp` and moves `releases/next.md`,
where each change adds what the player gets, to `releases/v<version>.md`, the release's notes.

## License

GPL-3.0. The game itself is not included and belongs to its owners.
