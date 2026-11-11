#!/usr/bin/env python3
"""
Road sprites: autotile sheets for each road tier, bridges and roundabouts.

Conventions match gen_terrain_sprites.py: a tile is a 32x16 iso diamond,
scaled 2x in game. Pixels off the road are transparent, so the natural
ground (grass, sand, water under a bridge) shows through.

Each tier sheet has 16 frames of 32x16, indexed by the road mask:
    NE = 1, SE = 2, SW = 4, NW = 8   (see game/entities/roads/RoadTile.hpp)

Tile-local coordinates: a pixel maps to (q, r) in [-0.5, 0.5]^2 where q
runs towards the SE edge and r towards the NE edge:
    pixel = centre + q * (16, 8) + r * (16, -8)
"""
import math
import os
from PIL import Image

OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "..", "assets", "sprites")

NE, SE, SW, NW = 1, 2, 4, 8
# direction bit -> (axis, sign); axis "r" for NE/SW, "q" for SE/NW
DIRS = {NE: ("r", 1), SE: ("q", 1), SW: ("r", -1), NW: ("q", -1)}

TIERS = {
    "dirt": dict(w=0.22, base=(150, 118, 80), dark=(120, 92, 60), light=(176, 146, 104)),
    "asphalt": dict(w=0.26, base=(78, 82, 90), dark=(60, 63, 70), light=(150, 154, 160)),
    "avenue": dict(w=0.38, base=(70, 74, 82), dark=(54, 57, 64), light=(168, 172, 178)),
}


def to_qr(px, py, cy=8):
    dx = px + 0.5 - 16.0
    dy = py + 0.5 - cy
    q = (dx / 16.0 + dy / 8.0) / 2.0
    r = (dx / 16.0 - dy / 8.0) / 2.0
    return q, r


def in_diamond(q, r):
    return abs(q) <= 0.5 and abs(r) <= 0.5


def noise(x, y, k=0):
    return ((x * 7 + y * 13 + k * 5) % 9) - 4


def road_hit(q, r, mask, w):
    """Return (lateral, longitudinal, in_centre) if (q, r) is on the road, else None."""
    if abs(q) <= w and abs(r) <= w:
        return (None, None, True)
    for bit, (axis, sign) in DIRS.items():
        if not (mask & bit):
            continue
        along, across = (q, r) if axis == "q" else (r, q)
        if along * sign >= 0 and abs(across) <= w:
            return (across, abs(along), False)
    return None


def paint_tier(name, spec, mask):
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    w = spec["w"]
    pad = mask == 0
    for py in range(16):
        for px in range(32):
            q, r = to_qr(px, py)
            if not in_diamond(q, r):
                continue
            ww = w + (0.08 if pad else 0.0)
            hit = road_hit(q, r, mask, ww)
            if hit is None:
                continue
            lat, lon, centre = hit
            n = noise(px, py)
            col = tuple(max(0, min(255, c + n)) for c in spec["base"])
            if name == "dirt":
                # Worn wheel ruts along each arm
                if lat is not None and abs(abs(lat) - 0.1) < 0.035:
                    col = spec["dark"]
                elif lat is not None and abs(lat) > w - 0.04:
                    col = spec["light"]
            elif name == "asphalt":
                if lat is not None and abs(lat) > w - 0.035:
                    col = spec["light"]                          # kerb
                elif lat is not None and abs(lat) < 0.04 and int(lon * 10) % 2 == 0:
                    col = (226, 196, 70)                         # dashed centre line
            else:  # avenue
                if lat is not None and abs(lat) > w - 0.04:
                    col = spec["light"]                          # kerb
                elif lat is not None and abs(lat) < 0.055:
                    col = (88, 138, 70)                          # planted median
                elif lat is not None and abs(abs(lat) - 0.2) < 0.03 and int(lon * 10) % 2 == 0:
                    col = (220, 222, 226)                        # lane dashes
            img.putpixel((px, py), (*col, 255))
    return img


def paint_roundabout(mask):
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    spec = TIERS["asphalt"]
    for py in range(16):
        for px in range(32):
            q, r = to_qr(px, py)
            if not in_diamond(q, r):
                continue
            rad = math.hypot(q, r)
            n = noise(px, py, 3)
            on_arm = road_hit(q, r, mask, spec["w"]) is not None and rad > 0.2
            if rad < 0.16:
                col = (84, 150, 72) if rad < 0.12 else (200, 200, 196)   # island + kerb
            elif rad < 0.42 or on_arm:
                col = tuple(max(0, min(255, c + n)) for c in spec["base"])
                if abs(rad - 0.29) < 0.025 and int(math.atan2(r, q) * 4) % 2 == 0:
                    col = (220, 222, 226)                                # ring lane dashes
                if 0.40 <= rad < 0.42 and not on_arm:
                    col = spec["light"]
            else:
                continue
            img.putpixel((px, py), (*col, 255))
    return img


def paint_bridge(name, axis):
    """32x24: deck in the top 16 rows, pillars hanging into the water below."""
    spec = TIERS[name]
    img = Image.new("RGBA", (32, 24), (0, 0, 0, 0))
    mask = (NE | SW) if axis == 0 else (SE | NW)
    w = spec["w"]
    # Pillars under the deck centre
    for py in range(9, 24):
        for px in (14, 15, 16, 17):
            shade = 120 if px in (14, 15) else 92
            img.putpixel((px, py), (shade, shade - 4, shade - 10, 255))
    for py in range(16):
        for px in range(32):
            q, r = to_qr(px, py)
            if not in_diamond(q, r):
                continue
            hit = road_hit(q, r, mask, w + 0.08)
            if hit is None:
                continue
            lat, lon, centre = hit
            across = lat if lat is not None else (r if axis == 0 else q)
            if abs(across) > w:
                col = (150, 104, 62) if name == "dirt" else (186, 188, 192)   # railing
            else:
                n = noise(px, py, 1)
                col = tuple(max(0, min(255, c + n)) for c in spec["base"])
                if name != "dirt" and abs(across) < 0.04 and int((lon or 0) * 10) % 2 == 0:
                    col = (226, 196, 70)
            img.putpixel((px, py), (*col, 255))
    return img


def on_grass(img):
    """Card thumbnail: the sprite over a grass diamond."""
    out = Image.new("RGBA", img.size, (0, 0, 0, 0))
    off = img.size[1] - 16
    for py in range(16):
        for px in range(32):
            q, r = to_qr(px, py)
            if in_diamond(q, r):
                out.putpixel((px, py + off), (82 + noise(px, py), 156 + noise(px, py, 2), 72, 255))
    out.alpha_composite(img)
    return out


def build_all():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    out = {}
    for name, spec in TIERS.items():
        sheet = Image.new("RGBA", (32 * 16, 16), (0, 0, 0, 0))
        for mask in range(16):
            sheet.paste(paint_tier(name, spec, mask), (mask * 32, 0))
        out[f"road_{name}.png"] = sheet
        out[f"road_{name}_icon.png"] = on_grass(paint_tier(name, spec, NE | SW))

    ring = Image.new("RGBA", (32 * 16, 16), (0, 0, 0, 0))
    for mask in range(16):
        ring.paste(paint_roundabout(mask), (mask * 32, 0))
    out["roundabout.png"] = ring
    out["roundabout_icon.png"] = on_grass(paint_roundabout(15))

    # Bridge frames: (tier * 2 + axis)
    bridge = Image.new("RGBA", (32 * 6, 24), (0, 0, 0, 0))
    for t, name in enumerate(TIERS):
        for axis in (0, 1):
            bridge.paste(paint_bridge(name, axis), ((t * 2 + axis) * 32, 0))
    out["road_bridge.png"] = bridge

    for filename, img in out.items():
        path = os.path.join(OUTPUT_DIR, filename)
        img.save(path)
        print(f"Generated {filename} ({img.size[0]}x{img.size[1]})")


if __name__ == "__main__":
    build_all()
