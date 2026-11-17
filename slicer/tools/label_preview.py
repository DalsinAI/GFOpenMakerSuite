#!/usr/bin/env python3
"""Labels a host preview (openmake-slice PREVIEW=...ppm) as a host render, with the
colour key, as a PNG for a report. Needs Pillow and DejaVu fonts.
  label_preview.py in.ppm out.png "what it shows"
MIT, Copyright (c) 2026 AmiCradle Team."""
import sys
from PIL import Image, ImageDraw, ImageFont
src, dst, line2 = sys.argv[1], sys.argv[2], sys.argv[3]
im = Image.open(src).convert('RGB'); W, H = im.size
# A narrow picture (a window) gets a wider sheet, so the label is never cut.
W = max(W, 1060)
out = Image.new('RGB', (W, H + 118), (245, 245, 245)); out.paste(im, (0, 118))
d = ImageDraw.Draw(out)
f = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', 24)
g = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 17)
d.text((14, 10), "HOST RENDER (Linux, CPU drawer) - not Amiga, not OpenGPU", font=f, fill=(180, 20, 20))
d.text((14, 50), line2, font=g, fill=(20, 20, 20))
x = 14
for name, c in [("outer wall", (0xe8, 0x74, 0x2a)), ("inner walls", (0xf0, 0xc0, 0x40)), ("skin", (0xd0, 0x30, 0x40)), ("infill", (0xb0, 0x80, 0x50)), ("brim", (0x30, 0xa0, 0x60))]:
    d.rectangle((x, 84, x + 20, 104), fill=c); d.text((x + 28, 84), name, font=g, fill=(20, 20, 20)); x += 28 + int(d.textlength(name, font=g)) + 24
d.text((x + 6, 84), "lower layers darker; travels not drawn", font=g, fill=(90, 90, 90))
out.save(dst)
