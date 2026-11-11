// Widescreen: the engine's (lib/turok-engine/src/widescreen.cpp, docs/widescreen.md) at this game's places, which
// us.toml's hooks say. The game's code takes its graphics struct as an argument and writes its display list through
// the head in it, at +0x1F810, so the frame's hook gives the engine the head from there each frame.

#include <cstdint>

#include "turok_engine.h"

// The frame (func_800023EC) starts, with its graphics struct.
extern "C" void t1_frame_begin(uint8_t* rdram, uint32_t graphics) {
    turok::set_display_list_head(graphics + 0x1F810);
    turok_frame_begin(rdram);
}
