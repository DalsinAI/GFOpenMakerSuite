#!/usr/bin/env python3
"""Labels a host preview of the OpenMakeBurn window as a host render.
  label_preview.py in.png out.png "what it shows" [batch]
batch: the view is the OpenGPU batch's triangles filled on the host.
MIT, Copyright (c) 2026 AmiCradle Team."""
import sys
from PIL import Image, ImageDraw, ImageFont

src, dst, line2 = sys.argv[1], sys.argv[2], sys.argv[3]
im = Image.open(src).convert("RGB")
W, H = im.size
W = max(W, 1060)
out = Image.new("RGB", (W, H + 118), (245, 245, 245))
out.paste(im, (0, 118))
d = ImageDraw.Draw(out)
f = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24)
g = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 17)
batch = len(sys.argv) > 4 and sys.argv[4] == "batch"
d.text(
    (14, 10),
    "HOST RENDER (Linux): OpenGPU batch, host-filled - not Amiga or a chip"
    if batch
    else "HOST RENDER (Linux, CPU drawer) - not Amiga, not OpenGPU",
    font=f,
    fill=(180, 20, 20),
)
d.text((14, 50), line2, font=g, fill=(20, 20, 20))
d.rectangle((14, 84, 34, 104), fill=(0x00, 0x67, 0xC0))
d.text((42, 84), "laser cut (shared preview colour); travels not drawn", font=g, fill=(20, 20, 20))
out.save(dst)
