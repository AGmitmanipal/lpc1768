"""Patch the LPC17xx vector checksum in a raw little-endian flash image."""
import pathlib
import struct
import sys

def patch(data):
    image = bytearray(data)
    if len(image) < 32:
        raise ValueError('Image is shorter than its vector table')
    words = struct.unpack_from('<8I', image)
    checksum = (-sum(words[:7])) & 0xffffffff
    struct.pack_into('<I', image, 28, checksum)
    assert sum(struct.unpack_from('<8I', image)) & 0xffffffff == 0
    return bytes(image)

if __name__ == '__main__':
    path = pathlib.Path(sys.argv[1])
    path.write_bytes(patch(path.read_bytes()))
    print('LPC17xx boot checksum patched and verified:', path)
