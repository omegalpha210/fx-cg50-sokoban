"""Check the real package and failure paths of its independent validator."""
import contextlib
import importlib.util
import io
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('verify_g3a', ROOT / 'tools/verify_g3a.py')
validator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validator)

class PackageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw = (ROOT / 'dist/SOKOBAN.g3a').read_bytes()
        (ROOT / 'build-host').mkdir(exist_ok=True)

    def verify_bytes(self, data):
        with tempfile.TemporaryDirectory(dir=ROOT / 'build-host') as work:
            path = Path(work) / 'test.g3a'
            path.write_bytes(data)
            with contextlib.redirect_stdout(io.StringIO()):
                validator.verify(path)

    def test_real_container(self):
        self.verify_bytes(self.raw)

    def test_truncated(self):
        for length in (0, 31, 0x7000, len(self.raw)-1):
            with self.subTest(length=length), self.assertRaises(ValueError):
                self.verify_bytes(self.raw[:length])

    def test_header_body_and_footer_corruption(self):
        for offset in (0, 8, 0x10, 0x20, 0x40, 0x60, 0x7100, len(self.raw)-1):
            data = bytearray(self.raw)
            data[offset] ^= 0x40
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                self.verify_bytes(data)

    def test_identity_even_with_recalculated_checksum(self):
        data = bytearray(self.raw)
        data[0x60:0x6b] = b'@OTHER\0\0\0\0\0'
        crc = ((sum(data[:32]) + sum(data[0x24:-4])) & 0xffffffff).to_bytes(4, 'big')
        data[0x20:0x24] = crc
        data[-4:] = crc
        with self.assertRaisesRegex(ValueError, 'internal identity'):
            self.verify_bytes(data)

    def test_stale_version_even_with_valid_checksum(self):
        data = bytearray(self.raw)
        data[0x130:0x13a] = b'00.01.0003'
        crc = ((sum(data[:32]) + sum(data[0x24:-4])) & 0xffffffff).to_bytes(4, 'big')
        data[0x20:0x24] = data[-4:] = crc
        with self.assertRaisesRegex(ValueError, 'release version'):
            self.verify_bytes(data)

if __name__ == '__main__':
    unittest.main()
