#!/usr/bin/env python3
"""Extract a raw .bin from a UF2 firmware file.

UF2 format: https://github.com/microsoft/uf2
Each 512-byte block has a 32-byte header followed by up to 476 bytes of data.
Data blocks are placed at their target flash address.
"""
import struct, sys

def uf2_to_bin(uf2_path, bin_path=None):
    if bin_path is None:
        bin_path = uf2_path.rsplit('.', 1)[0] + '.bin' if '.uf2' in uf2_path else uf2_path + '.bin'

    chunks = []
    with open(uf2_path, 'rb') as f:
        while True:
            block = f.read(512)
            if not block:
                break
            magic0, magic1 = struct.unpack_from('<II', block, 0)
            if magic0 != 0x0A324655 or magic1 != 0x9E5D5157:
                continue
            flags, addr, size = struct.unpack_from('<III', block, 8)
            if flags & 0x8000:  # no-flash flag
                continue
            chunks.append((addr, block[32:32+size]))

    if not chunks:
        sys.exit("No UF2 data blocks found")

    base = min(a for a, _ in chunks)
    end = max(a + len(d) for a, d in chunks)
    data = bytearray(end - base)
    for addr, chunk in chunks:
        offset = addr - base
        data[offset:offset+len(chunk)] = chunk

    # Trim trailing zeros
    while data and data[-1] == 0:
        data.pop()

    with open(bin_path, 'wb') as f:
        f.write(data)
    print(f"{uf2_path} -> {bin_path} ({len(data)} bytes)")

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} input.uf2 [output.bin]")
        sys.exit(1)
    uf2_to_bin(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else None)
