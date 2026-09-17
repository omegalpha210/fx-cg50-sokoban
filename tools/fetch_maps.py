#!/usr/bin/env python3
"""Explicitly acquire the pinned upstream inputs for a local build.

This command is never called implicitly by a build. It downloads no executable.
Map redistribution permission remains unconfirmed; see ASSET_PROVENANCE.md.
"""
import hashlib
import json
from pathlib import Path
import tempfile
from urllib.request import urlopen, Request

ROOT = Path(__file__).resolve().parents[1]

def fetch(root=ROOT):
    manifest = json.loads((root / 'assets/maps/manifest.json').read_text())
    destination = root / 'assets/maps/upstream'
    destination.mkdir(parents=True, exist_ok=True)
    revision = manifest['revision']
    for record in manifest['files']:
        name = record['path']
        if Path(name).name != name:
            raise ValueError('Manifest target must be a filename')
        expected = f'https://raw.githubusercontent.com/begoon/sokoban-maps/{revision}/{record["upstream_path"]}'
        if record['url'] != expected:
            raise ValueError('Manifest URL must use the pinned upstream revision')
        target = destination / name
        if target.exists() and hashlib.sha256(target.read_bytes()).hexdigest() == record['sha256']:
            print(f'Already verified: {name}')
            continue
        request = Request(expected, headers={'User-Agent': 'fx-cg50-sokoban-local-build'})
        with urlopen(request, timeout=30) as response:
            data = response.read(record['bytes'] + 1)
        if len(data) != record['bytes'] or hashlib.sha256(data).hexdigest() != record['sha256']:
            raise ValueError(f'Pinned size/SHA256 mismatch: {name}')
        with tempfile.NamedTemporaryFile(dir=destination, delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(data)
        temporary.replace(target)
        print(f'Downloaded and verified: {name}')
    print('Pinned inputs ready for local import. Redistribution permission remains unconfirmed.')

if __name__ == '__main__':
    fetch()
