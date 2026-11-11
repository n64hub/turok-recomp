#include "turok1.h"
#include "turok_engine.h"
#include "../../RecompiledFuncs/recomp_overlays.inl"

#include "librecomp/overlays.hpp"

// The game's code sections, from N64Recomp's section table (syms/, lib/turok-engine/tools/gen_syms.py): main, the first
// MB after the header, and ovl, the second block, both where the code runs through the TLB, and main's kseg0 view,
// which holds no functions but the jump tables the code reads.
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

// The engine maps both code sections at their TLB addresses and fills them from the ROM (lib/turok-engine/src/
// tlb_views.cpp), before the game runs. t2_VmInit maps the second block as 512 KB from 0x00400000, so the VM server
// hands its ROM regions (the audio banks) out from 0x00480000 on.
void t1::map_code_sections(uint8_t* rdram) {
    turok::map_tlb_sections(rdram, section_table, ARRLEN(section_table));
    turok::set_vm_base(0x00480000);
}
