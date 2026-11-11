#ifndef __TUROK1_H__
#define __TUROK1_H__

#include <cstdint>
#include <string_view>

namespace t1 {
    inline constexpr std::u8string_view program_id = u8"TurokRecompiled";
    inline constexpr std::string_view program_name = "Turok 2: Recompiled";

    // The game's code sections (register_overlays.cpp) and the patches over it (register_patches.cpp).
    void register_overlays();
    void register_patches();

    // The settings (config.cpp): RecompFrontend's tabs.
    void init_config();

    // Maps the code at the TLB addresses it runs at, through the engine (register_overlays.cpp), before the game runs.
    void map_code_sections(uint8_t* rdram);
}

#endif
