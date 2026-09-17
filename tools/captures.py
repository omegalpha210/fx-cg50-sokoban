#!/usr/bin/env python3
"""Convert actual host renderer PPM output; montage only, no mockup drawing."""
from pathlib import Path
from PIL import Image, ImageDraw
ROOT = Path(__file__).resolve().parents[1]
source = ROOT / 'build-host/captures'
target = ROOT / 'docs/captures'
target.mkdir(parents=True, exist_ok=True)
for p in sorted(source.glob('*.ppm')):
    with Image.open(p) as im: im.save(target / (p.stem+'.png'))
sheet = Image.new('RGB',(396*5,246*12),'#e5e9e7')
draw = ImageDraw.Draw(sheet)
for n in range(60):
    x,y=(n%5)*396,(n//5)*246
    with Image.open(target / f'play-{n+1:02}.png') as im: sheet.paste(im,(x,y))
    draw.text((x+8,y+227),f'LEVEL {n+1}',fill='black')
sheet.save(target / 'all-60-contact-sheet.png')
review = ['main','levels-group-1','levels-group-2','levels-group-3','levels-group-4','play-01','play-16','play-31','play-46','play-59','goals-and-crates','player-on-goal-fixture','init-confirm','congratulations-fixture','level60-complete-fixture','save-error','undo-before','undo-after','progress-restored','hud-max-counters-fixture','main-right-before-row','main-right-after-row','levels-1-right-before-row','levels-1-right-after-row']
sheet=Image.new('RGB',(396*4,246*((len(review)+3)//4)),'#e5e9e7');draw=ImageDraw.Draw(sheet)
for n,name in enumerate(review):
    x,y=(n%4)*396,(n//4)*246
    with Image.open(target / (name+'.png')) as im: sheet.paste(im,(x,y))
    draw.text((x+8,y+227),name,fill='black')
sheet.save(target / 'review-contact-sheet.png')
print(f'Converted {len(list(source.glob("*.ppm")))} shared-renderer captures and 2 contact sheets.')
