#!/usr/bin/env python3
"""
Extra building variants by recolouring the existing ones.

For res / com / ind tiers 1-3 writes v3 (from v1) and v4 (from v2) into
assets/sprites: saturated pixels (roofs, painted walls, awnings) get their
hue rotated and brightness nudged; greys, glass and outlines stay as they
are, so the new variants keep the same pixel-art style.
"""
import colorsys
import os
from PIL import Image

SPRITES = os.path.join(os.path.dirname(__file__), "..", "assets", "sprites")

# Per new variant: hue shift (turns), value scale, saturation scale
SHIFTS = {3: (0.55, 0.92, 0.75), 4: (0.10, 1.0, 0.85)}


def recolour(img, hue_shift, value_scale, sat_scale):
    img = img.convert("RGBA")
    px = img.load()
    for y in range(img.height):
        for x in range(img.width):
            r, g, b, a = px[x, y]
            if a == 0:
                continue
            h, s, v = colorsys.rgb_to_hsv(r / 255.0, g / 255.0, b / 255.0)
            if s < 0.22 or v < 0.15:                 # greys, outlines, dark windows: keep
                continue
            if 0.5 < h < 0.62 and v > 0.7:           # light blue glass: keep
                continue
            h = (h + hue_shift) % 1.0
            v = max(0.0, min(1.0, v * value_scale))
            s = max(0.0, min(1.0, s * sat_scale))
            nr, ng, nb = colorsys.hsv_to_rgb(h, s, v)
            px[x, y] = (int(nr * 255), int(ng * 255), int(nb * 255), a)
    return img


def build_all():
    made = 0
    for prefix in ("res", "com", "ind"):
        for tier in (1, 2, 3):
            for new, base in ((3, 1), (4, 2)):
                src = os.path.join(SPRITES, "%s_t%d_v%d.png" % (prefix, tier, base))
                if not os.path.exists(src):
                    continue
                hue, val, sat = SHIFTS[new]
                recolour(Image.open(src), hue, val, sat).save(
                    os.path.join(SPRITES, "%s_t%d_v%d.png" % (prefix, tier, new)))
                made += 1
    print(f"Generated {made} building variants -> {os.path.abspath(SPRITES)}")


if __name__ == "__main__":
    build_all()
