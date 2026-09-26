#!/usr/bin/env python3
"""Generate the original player-palette audit; README gameplay uses showcase.py."""
from pathlib import Path
from PIL import Image, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
def main():
    target=ROOT/'docs/public-captures';target.mkdir(parents=True,exist_ok=True)
    with Image.open(ROOT/'build-host/public-captures/player-sizes.ppm') as im:
        sheet=im.crop((0,0,396,206));draw=ImageDraw.Draw(sheet)
        for group,name in enumerate(('BASIC','INTERMEDIATE','ADVANCED','MASTER')):
            draw.text((8,3+group*50),name,fill='black')
            for size in range(9,20):
                draw.text((8+(size-9)*35,40+group*50),str(size)+'px',fill='black')
        sheet.save(target/'player-sizes.png')
        sheet.resize((1188,618),Image.Resampling.NEAREST).save(target/'player-sizes-3x.png')
    print('Player palette: four groups at every tile size; fixture gameplay is not published.')
if __name__=='__main__':main()
