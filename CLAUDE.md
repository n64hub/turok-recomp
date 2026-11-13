# CLAUDE.md

A static recompilation of the N64 *Turok: Dinosaur Hunter* (USA 1.0) with N64Recomp, N64ModernRuntime, RecompFrontend
and RT64. The functions are found in the ROM, with no decompilation. Almost all of it is written by AI agents, so this
file is what the next one needs: the rules, where things are, and the facts about the game the code can't say on its
own.

## What lives where

This repo is the game: its symbols, its recompilation, the patches over it and the desktop build. What the Turok games
share is `lib/turok-engine` (n64hub/turok-engine, its README says what is there and how a game uses it): change it
there, then move this repo's submodule onto the commit. Turok 1 came before the engine family's TLB-mapped code, so it
takes from the engine only what fits (the finder, the Controller Pak, the FPU mode, osClockRate, widescreen).

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
  code's ROM read (`boot.cpp`), the natives the patches call, and the frame's hook for the engine's widescreen
  (`widescreen.cpp`).
- `lib/`: `turok-engine`, upstream `rt64`, `N64ModernRuntime` and `RecompFrontend` at the commits N64Recomp-browser's
  web forks are built on, and `N64Recomp-browser`, whose `patches/` `tools/build_n64recomp.sh` and `CMakeLists.txt`
  apply to the other three. `runtime-patches/` are this game's own over them.
  A checkout gets them with turok-engine's `tools/submodules.sh`, from a cache its checkouts on the machine share:
  `bash lib/turok-engine/tools/submodules.sh` after `git submodule update --init lib/turok-engine` (at a pin that has
  it), or that script from any turok-engine checkout, given this one's path. Never `git submodule update --init
  --recursive`: it downloads them all again, about 20 minutes, most of it rt64's `contrib/dxc`.
- `icons/`: drawn by `tools/icon.ps1` from plain text and shapes.
- The web page: the `EMSCRIPTEN` branches of `CMakeLists.txt`, `tools/build_web.sh` (in `tools/docker/Dockerfile.web`)
  and the shader list `src/rt64_shaders.txt`, recorded by `tools/shader_plan.json`; `tools/play_plan.json` is the
  page's play, and `tools/survey.js` reads the main loop's state for both (`docs/web.md`).
- Releases: `tools/release.sh` (Linux in `tools/docker/Dockerfile.linux`), each release's notes in `releases/`
  (README, Releases). Each change a player would notice adds a line to `releases/next.md`, and every fix is released:
  the work that lands one cuts the release once it merges, unasked, or waits for one already in flight, which carries
  it (N64Recomp-browser's CLAUDE.md, Releasing). A release moves `releases/next.md` to `releases/v<version>.md`.
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
- It walks and strafes on the C buttons and turns and looks with the stick; Z fires, R jumps, A and B change weapon
  (seen in the first level with `--input`), as the later games do. It looks down with the stick pushed up, as a
  plane's stick (seen on the page). `src/game/config.cpp` binds a shooter's layout to them, the right stick and the up
  arrow pushing the stick down so that up looks up, and `src/main/main.cpp` turns the mouse into the stick, the same
  way round, and its left button into Z.
- Its code is compiled for the FPU's FR=0 mode (32-bit registers, a double in an even/odd pair: no double operation
  names an odd register, and mtc1/mfc1 move the halves), the runtime's default, unlike the later games: it doesn't call
  the engine's `turok::set_fpu_mode`. In FR=1 its doubles read wrong (the legal screen's timer reads 0 and the screen
  restarts for good).
- Its audio thread makes audio frames for as long as osAiGetStatus doesn't report the FIFO full, so osAiGetStatus is
  the engine's `turok_ai_get_status`; with the runtime's, which never reports it full, the thread starves the main one.
- Its RCP scheduler (libultra's sample scheduler, messages 666 to 669) crashes on purpose, a store to address 0, when
  160 retraces pass with no graphics task done. `us.toml` makes that store a nop, since here a frame that long is a
  shader build or a slow GPU, not a hung RCP; so a hang before the first frame shows as a black window, not a crash.
- Its graphics code is handed a graphics struct (the frame's func_800023EC takes it in $a0) and writes the display list
  through the head in it, at +0x1F810, for the F3DEX microcode (GBI 1). The struct is double-buffered: the frame swaps
  it in its first calls, so the frame's widescreen hook sits after them (`us.toml`).
- Its threads: 1 idle, 3 audio, 4 the scheduler, 5 the fault handler, 6 the main loop (`0x80008604`, its state machine
  on the word at 0x80119534).
