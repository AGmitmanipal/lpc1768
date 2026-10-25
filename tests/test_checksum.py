import importlib.util
import pathlib
import struct
root = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('checksum', root/'tools/checksum.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
source = struct.pack('<9I', 0x10008000, 0x101, 0x151, 0x151, 0x151, 0x151, 0x151, 0, 0xAABBCCDD)
result = module.patch(source)
assert sum(struct.unpack_from('<8I', result)) & 0xffffffff == 0
assert result[32:] == source[32:]
assert module.patch(result) == result
try:
    module.patch(b'bad')
except ValueError:
    pass
else:
    raise AssertionError('Truncated image accepted')
print('PASS: boot checksum, idempotence, payload preservation, short-image rejection')
