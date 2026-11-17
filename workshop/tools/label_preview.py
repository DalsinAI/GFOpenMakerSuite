#!/usr/bin/env python3
"""Labels a host preview of the OpenMakeWorkshop window as a host render.
  label_preview.py in.png out.png "what it shows"
MIT, Copyright (c) 2026 AmiCradle Team."""
import sys
from PIL import Image, ImageDraw, ImageFont

src, dst, line2 = sys.argv[1], sys.argv[2], sys.argv[3]
im = Image.open(src).convert("RGB")
W, H = im.size
W = max(W, 1060)
out = Image.new("RGB", (W, H + 96), (245, 245, 245))
out.paste(im, (0, 96))
d = ImageDraw.Draw(out)
f = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24)
g = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 17)
d.text(
    (14, 10),
    "HOST RENDER (Linux, CPU drawer) - not Amiga, not OpenGPU",
    font=f,
    fill=(180, 20, 20),
)
d.text((14, 50), line2, font=g, fill=(20, 20, 20))
out.save(dst)
