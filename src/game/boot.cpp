// The boot code's ROM read (t1_boot_rom_read, 0x80000450): it copies words straight off the cartridge bus, from
// osRomBase | rom, polling the PI's status between them, which reads nothing in the recompiled code. The boot code calls
// it twice (us.toml lists it in ignored): for the packed main program's RNC header, then for the whole block, which
// it unpacks to 0x80001300.

#include "recomp.h"
#include "librecomp/game.hpp"

// void t1_boot_rom_read(u32 rom, void* ram, u32 size)
extern "C" void t1_boot_rom_read(uint8_t* rdram, recomp_context* ctx) {
    uint32_t rom = (uint32_t)ctx->r4;
    gpr ram = ctx->r5;
    uint32_t size = (uint32_t)ctx->r6 & ~3u;
    recomp::do_rom_read(rdram, ram, 0x10000000u + rom, size);
}
