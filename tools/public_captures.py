#!/usr/bin/env python3
"""Convert shared-renderer captures of original fixtures for public docs."""
from pathlib import Path
from PIL import Image, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
def main():
    target=ROOT/'docs/public-captures'
    target.mkdir(parents=True,exist_ok=True)
    names=['main','level-select','gameplay-original-fixture','completed-original-fixture','confirmation-original-fixture']
    sheet=Image.new('RGB',(792,3*246),'#e5e9e7');draw=ImageDraw.Draw(sheet)
    for i,name in enumerate(names):
        with Image.open(ROOT/'build-host/public-captures'/f'{name}.ppm') as im:
            im.save(target/f'{name}.png')
            x,y=(i%2)*396,(i//2)*246;sheet.paste(im,(x,y))
            draw.text((x+8,y+228),name,fill='black')
    sheet.save(target/'overview.png')
    print('Public figures: actual renderer, independently authored illustration fixture.')
if __name__=='__main__': main()
