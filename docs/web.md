# The web page

The game plays in a browser: built with Emscripten on N64Recomp-browser's `frontend/` into `TurokRecompiled.html`
(the launcher, the recomp's menus, SDL2's audio and keyboard, the ROM from Load ROM, and the saves kept in IndexedDB;
`lib/N64Recomp-browser/frontend/CMakeLists.txt`'s header says what each part does), drawn through WebGPU. The page is
built from your ROM like the desktop program, so it is never built in CI and never published with the game in it.

What is the game's own stays here: the `EMSCRIPTEN` branches of `CMakeLists.txt` that make the game a page,
`tools/build_web.sh` and the shader list (below). The rest is N64Recomp-browser's: `docs/porting.md` (WebGPU and the
shaders, prebuilt shader lists, Emscripten), `docs/playing.md` (playing and measuring a page), `docs/forks.md` and
`docs/findings.md`. Every play of this game's page is a line in N64Recomp-browser's `plays.jsonl`, never a record here.

All of the game's code and data are in kseg0 (CLAUDE.md, Facts about the game), so unlike the later Turok games it
needs no window for TLB-mapped addresses (`RECOMP_KUSEG_WINDOW`).

## Build it

On Linux with Docker, with the ROM at `roms/turok1.us.z64`:

```bash
tools/build_web.sh                  # [cpus] [cmake arguments]
```

It builds `build/web/TurokRecompiled.html` with its `.js`, `.wasm` and `.data` (the recomp's fonts and menus,
nothing of the game) in `tools/docker/Dockerfile.web`, the image every N64Recomp-browser page is built in, with ccache
in `build/ccache-web`. The script's header says each step. Two things differ from the desktop build:

- The patches library (`patches/`, built for MIPS) is recompiled by the native N64Recomp that `tools/recompile.sh`
  builds, not the runtime's, which Emscripten would build for the browser.
- The generated code and the hooks in `us.toml` are compiled with implicit function declarations as errors, so a hook
  declares any native it calls.

Serve `build/web` over HTTP from localhost (any static server; the page brings its own `coi_serviceworker.js` for the
headers threads need) and open `TurokRecompiled.html` in Chrome or Firefox with WebGPU, then Load ROM.
