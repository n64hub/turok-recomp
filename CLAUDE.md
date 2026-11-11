# CLAUDE.md

A static recompilation of the N64 *Turok: Dinosaur Hunter* (USA 1.0) with N64Recomp, N64ModernRuntime, RecompFrontend
and RT64. The functions are found in the ROM, with no decompilation. Almost all of it is written by AI agents, so this
file is what the next one needs: the rules, where things are, and the facts about the game the code can't say on its
own.

## What lives where

This repo is the game: its symbols, its recompilation, the patches over it and the desktop build. What the Turok games
share is `lib/turok-engine` (n64hub/turok-engine, its README says what is there and how a game uses it): change it
there, then move this repo's submodule onto the commit. Turok 1 came before the engine family's TLB-mapped code, so it
takes from the engine only what fits (the finder, the Controller Pak, the FPU mode, osClockRate).

- `tools/unpack_main.py`: unpacks the main program (RNC method 1) from `roms/turok1.us.z64` into
  `roms/turok1.us.recomp.z64`, which is what the finder, N64Recomp and RSPRecomp read. `tools/recompile.sh` runs it.
- `syms/layout.toml`: where the code is (the boot code at 0x80000400, the main program at 0x80001300).
  `syms/names.toml`: the names known, kept for good. `syms/turok1.us.syms.toml`: written from both and the ROM by
  `lib/turok-engine/tools/gen_syms.py`, never by hand.
- `us.toml`: N64Recomp's configuration, with the stubs, the natives (`ignored`) and the hooks; `tools/recompile.sh`
  writes `RecompiledFuncs/` with it, and the audio microcode with `aspMain.us.toml`.
- `patches/` and `patches.toml`: MIPS C recompiled into PatchesLib. `ui_funcs.h`, `patch_helpers.h` and
  `recompui_event_structs.h` are RecompFrontend's.
- `src/main/main.cpp`: RecompFrontend's main with the GameEntry. `src/main/register_overlays.cpp`: the code sections,
  and the main program's functions registered where the boot code unpacks it. `src/game/`: the settings, the boot
  code's ROM read (`boot.cpp`) and the natives the patches call.
- `lib/`: `turok-engine`, upstream `rt64`, `N64ModernRuntime` and `RecompFrontend` at the commits N64Recomp-browser's
  web forks are built on, and `N64Recomp-browser`, whose `patches/` `tools/build_n64recomp.sh` and `CMakeLists.txt`
  apply to the other three. `runtime-patches/` are this game's own over them.
- `icons/`: drawn by `tools/icon.ps1` from plain text and shapes.
- Comments beside the code, for how and why: each file's header. Read the header of the file you change and update
  it in the same commit.

## Rules

- No ROMs, ROM-derived assets or extracted game data in git, CI or PRs (`tools/ci/check_repo.py` checks). Only the
  names, addresses and sizes in `syms/` and the layout file. The ROMs are `roms/turok1.us.z64` and the unpacked
  `roms/turok1.us.recomp.z64`, never committed.
- Nothing here comes from a decompilation of the game or from its original source: names are worked out from the
  ROM's code.
- CI builds everything but the ROM-generated sources, without linking, on Windows and Linux. turok-engine's CI calls
  it with `engine-ref` to build this repo's `main` against an engine commit.

## Facts about the game

- The IPL3 copies the first MB after the header to 0x80000400. Only its first 0xF00 bytes are code: the entry point,
  a ROM read straight off the cartridge bus (`t1_boot_rom_read`, a native here) and the RNC unpackers. It reads the
  main program from ROM 0x783690 (packed, RNC method 1), unpacks it to 0x80001300 (0xF7D20 bytes), clears its bss to
  0x8013A130 and jumps to it. All of the code runs in kseg0; nothing is TLB-mapped.
- The main program is in N64Recomp's section table at ROM 0x800000, past the real ROM's 8 MB, where
  `tools/unpack_main.py` puts it. No DMA ever reads that range, so on_init registers its functions
  (`t1::register_main_program`).
- Its libultra is older than n64sym's signatures for the functions the runtime replaces (osInitialize, the managers,
  osContInit, osPiStartDma, osAi*): `syms/names.toml` names them, found by their calls and registers.
- It does its DMA through libultra's PI manager (osPiStartDma), unlike the later games' own DMA threads.
- It saves to a Controller Pak in port 1.
