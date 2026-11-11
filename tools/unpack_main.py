"""Writes roms/turok1.us.recomp.z64, the ROM N64Recomp and the function finder read: the player's ROM with the game's
main program unpacked and put after it.

Turok: Dinosaur Hunter keeps its main program in the ROM as one block packed with Rob Northen's ProPack (RNC method 1).
The boot code the IPL3 loads (ROM 0x1000, run at 0x80000400) reads the block from the ROM, unpacks it to 0x80001300
and jumps there. The recompiler needs the unpacked code to translate, so this unpacks it once at build time and puts
it at MAIN_ROM, past the end of the 8 MB ROM, where syms/layout.toml says the main segment is. At run time the game
unpacks it from the player's own ROM, as on the console, and nothing reads MAIN_ROM.

The format: an 18-byte header ("RNC", the method, the unpacked and packed sizes, the CRC-16 of each, the leeway and
the chunk count, big-endian), then a bit stream read from the low bit up, out of little-endian 16-bit words. Each chunk
has three Huffman tables (literal run lengths, match distances, match lengths) and a count of runs; each run is a
literal length, that many bytes copied straight from the input (byte-aligned, between the bit words), then a match
(distance + 1, length + 2) unless it is the chunk's last run. A table entry's value v >= 2 stands for 2^(v-1) plus
v-1 more bits. The unpacked bytes are checked against the header's CRC-16 (reflected polynomial 0xA001).

Usage, from the repo's root: python3 tools/unpack_main.py [roms/turok1.us.z64]
"""
import hashlib
import struct
import sys
from pathlib import Path

ROM_SHA1 = "40fb0250c095740031278fb1f82b9937a3895e01"  # Turok: Dinosaur Hunter (USA 1.0), .z64
PACKED_MAIN = 0x783690  # the boot code's lui/addiu $a0, 0x783690
MAIN_ROM = 0x800000  # where the unpacked main goes in the recompiler's ROM: right after the 8 MB of the real one
MAIN_SIZE = 0xF7D20  # unpacked, run at 0x80001300


def crc16(data):
    table = []
    for i in range(256):
        c = i
        for _ in range(8):
            c = (c >> 1) ^ 0xA001 if c & 1 else c >> 1
        table.append(c)
    crc = 0
    for b in data:
        crc = table[(crc ^ b) & 0xFF] ^ (crc >> 8)
    return crc


class Bits:
    """The bit stream: `buf` holds `count` unread bits from the low end, refilled a 16-bit word at a time."""

    def __init__(self, data, pos):
        self.data = data
        self.pos = pos  # the word being read; literal bytes are read from here
        self.buf = self.word(pos)
        self.count = 16

    def word(self, p):  # 32 bits from p, little-endian (the bytes past the input read as 0)
        return int.from_bytes(self.data[p:p + 4].ljust(4, b"\0"), "little")

    def peek(self, n):
        return self.buf & ((1 << n) - 1)

    def advance(self, n):
        self.buf >>= n
        self.count -= n
        if self.count < 16:
            self.pos += 2
            self.buf |= self.word(self.pos) << self.count
            self.count += 16

    def read(self, n):
        v = self.peek(n)
        self.advance(n)
        return v

    def fix(self):
        """After a literal run moved pos past the copied bytes: the word at pos follows the bits still unread."""
        self.count -= 16
        self.buf &= (1 << self.count) - 1
        self.buf |= self.word(self.pos) << self.count
        self.count += 16


def read_table(bits):
    num = bits.read(5)
    lengths = [bits.read(4) for _ in range(num)]
    table, code = [], 0
    for length in range(1, 17):
        for value, l in enumerate(lengths):
            if l == length:
                rev = int(format(code, f"0{length}b")[::-1], 2)  # the codes are stored low bit first
                table.append((rev, length, value))
                code += 1
        code <<= 1
    return table


def read_value(bits, table):
    for code, length, value in table:
        if bits.peek(length) == code:
            bits.advance(length)
            if value >= 2:
                value = (1 << (value - 1)) | bits.read(value - 1)
            return value
    raise ValueError(f"no Huffman code matches at input offset {bits.pos:#x}")


def unpack(data, off):
    magic, method, size, packed, crc_unpacked, crc_packed = struct.unpack_from(">3sBIIHH", data, off)
    if magic != b"RNC" or method != 1:
        raise ValueError(f"no RNC method 1 block at {off:#x}")
    body = data[off + 18:off + 18 + packed]
    if crc16(body) != crc_packed:
        raise ValueError("the packed block's CRC doesn't match")
    out = bytearray()
    bits = Bits(body, 0)
    bits.advance(2)  # the lock and key flags
    while len(out) < size:
        raw, dist, length = read_table(bits), read_table(bits), read_table(bits)
        runs = bits.read(16)
        while True:
            n = read_value(bits, raw)
            if n:
                out += body[bits.pos:bits.pos + n]
                bits.pos += n
                bits.fix()
            runs -= 1
            if runs <= 0:
                break
            d = read_value(bits, dist) + 1
            n = read_value(bits, length) + 2
            for _ in range(n):
                out.append(out[-d])
    out = bytes(out[:size])
    if crc16(out) != crc_unpacked:
        raise ValueError("the unpacked main program's CRC doesn't match")
    return out


def main():
    src = Path(sys.argv[1] if len(sys.argv) > 1 else "roms/turok1.us.z64")
    rom = src.read_bytes()
    if hashlib.sha1(rom).hexdigest() != ROM_SHA1:
        sys.exit(f"{src} is not Turok: Dinosaur Hunter (USA 1.0) as a .z64 (SHA-1 {ROM_SHA1})")
    code = unpack(rom, PACKED_MAIN)
    assert len(code) == MAIN_SIZE and len(rom) == MAIN_ROM
    out = src.with_name("turok1.us.recomp.z64")
    new = rom + code
    if not out.exists() or out.read_bytes() != new:
        out.write_bytes(new)
    print(f"{out}: main program unpacked to {MAIN_ROM:#x}, {len(code):#x} bytes, SHA-1 {hashlib.sha1(new).hexdigest()}")


if __name__ == "__main__":
    main()
