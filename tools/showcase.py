#!/usr/bin/env python3
"""Convert the explicitly requested real-level README captures, without editing pixels."""
from pathlib import Path
import hashlib
import json
from PIL import Image
from public_snapshot import SHOWCASE_IMAGES
ROOT=Path(__file__).resolve().parents[1]
def main():
    target=ROOT/'docs/screenshots';target.mkdir(parents=True,exist_ok=True)
    manifest={'capture':'actual shared C renderer and unchanged pinned maps on host; not hardware photos',
              'levels':{'basic':1,'intermediate':16,'advanced':31,'master':59},
              'completion':{'level':1,'legal_moves':394,'pushes':134,'replay':'tests/showcase.c',
                            'from_original_start':True,'optimality_claimed':False},
              'map_rights':'Separate upstream rights; screenshots are not a map license or a redistribution grant.',
              'files':{}}
    for name in SHOWCASE_IMAGES:
        with Image.open(ROOT/'build-host/showcase'/f'{name}.ppm') as im:
            assert im.size==(396,224) and im.mode=='RGB'
            output=target/f'{name}.png';im.save(output)
        manifest['files'][output.name]=hashlib.sha256(output.read_bytes()).hexdigest()
    (target/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('README: 12 native-size PNGs from real maps; legal level-1 completion replay; no save-error image.')
if __name__=='__main__':main()
