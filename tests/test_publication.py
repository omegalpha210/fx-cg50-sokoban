"""Bounded public-export safety checks; no GitHub writes or credential access."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('public_snapshot',ROOT/'tools/public_snapshot.py')
public=importlib.util.module_from_spec(spec);spec.loader.exec_module(public)

class PublicationTests(unittest.TestCase):
    def test_sensitive_asset_paths_excluded(self):
        chosen={p.as_posix() for p in public.selected_files(ROOT)}
        for name in public.BANNED_EXACT:self.assertNotIn(name,chosen)
        self.assertFalse(any(p.startswith(public.BANNED_PREFIXES) for p in chosen))
        self.assertIn('tests/public/fixture.c',chosen)
        self.assertIn('src/game/game.c',chosen)
        self.assertIn('assets/maps/manifest.json',chosen)

    def test_map_pack_binary_and_pdf_rejected(self):
        for name in ['src/maps/generated_maps.c','dist/SOKOBAN.g3a','private.pdf']:
            with self.subTest(name=name),self.assertRaises(ValueError):
                public.audit_files(ROOT,[Path(name)])

    def test_secret_and_private_path_rejected(self):
        (ROOT/'.local').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=ROOT/'.local') as temp:
            root=Path(temp);file=root/'file.txt'
            for data in [b'gh' + b'p_' + b'x'*36, b'/' + b'Users' + b'/someone/private']:
                file.write_bytes(data)
                with self.assertRaises(ValueError):public.audit_files(root,[Path('file.txt')])
            file.write_text('Original source and documented upstream URL')
            public.audit_files(root,[Path('file.txt')])

    def test_existing_destination_never_overwritten(self):
        with self.assertRaisesRegex(ValueError,'must not exist'):
            public.export(ROOT,ROOT)

if __name__=='__main__':unittest.main()
