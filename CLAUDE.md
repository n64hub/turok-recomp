# CLAUDE.md

A static recompilation of the N64 *Turok 2: Seeds of Evil* (USA 1.0) with N64Recomp, N64ModernRuntime, RecompFrontend
and RT64. No decompilation exists: the functions are found in the ROM. Almost all of it is written by AI agents, so
this file is what the next one needs: the rules, where things are, and the facts about the game the code can't say on
its own.

## What lives where

This repo is the game: its symbols, its recompilation, the patches over it and the desktop build. What the Turok games
share is `lib/turok-engine` (n64hub/turok-engine, its README says what is there and how a game uses it): change it
there, then move this repo's submodule onto the commit.

- `syms/layout.toml`: where the game's code is in the ROM and where it runs (through the TLB, at 0x00200400 and
  0x00400000). `syms/names.toml`: the names known, kept for good, and the engine's natives (`turok_*`).
  `syms/turok2.us.syms.toml`: written from both and the ROM by `lib/turok-engine/tools/gen_syms.py`, never by hand.
- `us.toml`: N64Recomp's configuration, with the functions the engine replaces (`ignored`) and the hooks;
  `tools/recompile.sh` writes `RecompiledFuncs/` with it, and the audio microcode with `aspMain.us.toml`.
- `patches/` and `patches.toml`: MIPS C recompiled into PatchesLib. `ui_funcs.h`, `patch_helpers.h` and
  `recompui_event_structs.h` are RecompFrontend's.
- `src/main/main.cpp`: RecompFrontend's main with the GameEntry, which calls the engine at init, at each thread's
  creation and from the audio callbacks. `src/game/`: the settings and the natives the patches call.
- `lib/`: `turok-engine`, upstream `rt64`, `N64ModernRuntime` and `RecompFrontend` at the commits N64Recomp-browser's
  web forks are built on, and `N64Recomp-browser`, whose `patches/` `tools/build_n64recomp.sh` and `CMakeLists.txt`
  apply to the other three. `runtime-patches/` are this game's own over them.
- `icons/`: drawn by `tools/icon.ps1` from plain text and shapes.
- Comments beside the code, for how and why: each file's header. Read the header of the file you change and update
  it in the same commit.

## Rules

- No ROMs, ROM-derived assets or extracted game data in git, CI or PRs (`tools/ci/check_repo.py` checks). Only the
  names, addresses and sizes in `syms/` and the layout file. The ROM is `roms/turok2.us.z64`, never committed.
- CI builds everything but the ROM-generated sources, without linking, on Windows and Linux. turok-engine's CI calls
  it with `engine-ref` to build this repo's `main` against an engine commit.

## Facts about the game

- The boot code copies the first MB to 0x80000400 and the game maps it at 0x00200400 with a TLB entry; a second block
  is mapped at 0x00400000. The GameEntry's entrypoint is the kseg0 address (`turok::boot_load_address`); the engine
  opens the TLB pages and fills them (`turok::map_tlb_sections`).
- The game runs its FPU in FR mode (`turok::set_fpu_mode`), maps ROM regions through its own VM server
  (`turok_vm_map_rom_region`), runs its own PI DMA (`turok_pi_raw_start_dma`) and paces its audio thread by
  osAiGetStatus (`turok_ai_get_status`).
- It wants a Controller Pak in port 1 and asks osMotorInit about a Rumble Pak in each port (`turok_motor_init`):
  without the first it stops on "Controller Pak not found".
- The finder's mistakes show up as a stack that doesn't balance: a function split in two leaves $sp short after the
  call, and a crash far from it. A start found only by a jal in a data fragment split one function at its epilogue
  (gen_syms now drops those). Check a new syms file for functions whose `addiu $sp` don't balance.
