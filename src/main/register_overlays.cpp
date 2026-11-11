#include "turok1.h"
#include "turok_engine.h"
#include "../../RecompiledFuncs/recomp_overlays.inl"

#include "librecomp/overlays.hpp"

// The game's code sections, from N64Recomp's section table (syms/, lib/turok-engine/tools/gen_syms.py): boot, the code
// at the start of the first MB after the header, which the runtime loads at 0x80000400 as the IPL3 would, and main,
// the main program.
void t1::register_overlays() {
    recomp::overlays::overlay_section_table_data_t sections {
        .code_sections = section_table,
        .num_code_sections = ARRLEN(section_table),
        .total_num_sections = num_sections,
    };

    recomp::overlays::overlays_by_index_t overlays {
        .table = overlay_sections_by_index,
        .len = ARRLEN(overlay_sections_by_index),
    };

    recomp::overlays::register_overlays(sections, overlays);
}

// The main program is in the section table at ROM 0x800000, past the end of the real ROM, where tools/unpack_main.py
// puts it for the recompiler: the game itself unpacks it from the packed block at ROM 0x783690 into 0x80001300, so no
// DMA of the section's ROM range ever loads it. Its functions are registered at 0x80001300 here for the calls through
// pointers, as a DMA of it would.
void t1::register_main_program() {
    load_overlays(0x800000, (int32_t)0x80001300u, 0xF7D20);
}
