#!/usr/bin/env python3
"""
Natural terrain sprites for procedural world generation.

Generates only the terrain textures (sand, forest path, forest, rock) so it
never overwrites the building sprites produced by the other generators.
Same conventions as gen_sprites.py: 32x16 base diamond, tall sprites keep
their footprint diamond in the bottom 16 rows, scaled 2x in game.
"""
import os
from PIL import Image

OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "..", "assets", "sprites")


def diamond_rows(base_y=0):
    """Yield (y, x0, x1) spans of a 32x16 iso diamond starting at base_y."""
    for yr in range(16):
        hw = (yr + 1) * 2 if yr < 8 else (16 - yr) * 2
        yield base_y + yr, 16 - hw, 16 + hw


def make_sand():
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    for y, x0, x1 in diamond_rows():
        for x in range(x0, x1):
            v = ((x * 9 + y * 5) % 13) - 6
            speck = (x * 7 + y * 11) % 17 == 0
            if speck:
                img.putpixel((x, y), (200, 180, 130, 255))
            else:
                img.putpixel((x, y), (232 + v // 2, 212 + v, 158 + v, 255))
    return img


def make_forest_path():
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    for y, x0, x1 in diamond_rows():
        for x in range(x0, x1):
            v = ((x * 7 + y * 13) % 23) - 11
            # Worn earth trail running through the centre of a dark forest floor
            on_trail = abs((x - 16) - (y - 8) * 2) < 5 or abs((x - 16) + (y - 8) * 2) < 5
            if on_trail:
                img.putpixel((x, y), (150 + v // 2, 118 + v // 2, 78 + v // 3, 255))
            else:
                img.putpixel((x, y), (40 + v // 3, 112 + v // 2, 42 + v // 3, 255))
    return img


def draw_tree(img, cx, base, height, radius, shade):
    # Trunk
    for y in range(base - 3, base + 1):
        for x in (cx, cx + 1):
            img.putpixel((x, y), (92, 62, 38, 255))
    # Layered conifer canopy, lit from the north-west
    top = base - 3 - height
    for y in range(top, base - 2):
        t = (y - top) / max(1, height)
        hw = max(1, int(round(radius * t)))
        for x in range(cx - hw + 1, cx + hw + 1):
            if not (0 <= x < 32 and 0 <= y < img.size[1]):
                continue
            lit = x <= cx
            g = shade + (22 if lit else 0) + ((x * 3 + y * 5) % 7) - 3
            img.putpixel((x, y), (28 + (8 if lit else 0), g, 36, 255))


def make_forest():
    h = 32
    img = Image.new("RGBA", (32, h), (0, 0, 0, 0))
    # Back-to-front so nearer trees overlap farther ones
    draw_tree(img, 10, h - 10, 13, 5, 105)
    draw_tree(img, 21, h - 10, 15, 5, 98)
    draw_tree(img, 15, h - 6, 14, 6, 112)
    draw_tree(img, 6, h - 5, 10, 4, 100)
    draw_tree(img, 25, h - 5, 11, 4, 108)
    return img


def make_rock():
    h = 24
    img = Image.new("RGBA", (32, h), (0, 0, 0, 0))
    base = h - 16
    # Rubble ground diamond
    for y, x0, x1 in diamond_rows(base):
        for x in range(x0, x1):
            v = ((x * 11 + y * 7) % 19) - 9
            img.putpixel((x, y), (128 + v, 122 + v, 108 + v, 255))
    # Boulder mound rising above the diamond
    cx, cy = 16, base + 7
    for y in range(0, h):
        for x in range(32):
            dx = (x - cx) / 13.0
            dy = (y - cy) / 10.0
            if dx * dx + dy * dy <= 1.0 and y <= base + 11:
                lit = (x - cx) - (y - cy) < 0
                v = ((x * 5 + y * 9) % 11) - 5
                c = (150 + v, 146 + v, 136 + v) if lit else (104 + v, 100 + v, 94 + v)
                img.putpixel((x, y), (*c, 255))
    # Crack lines for texture
    for x, y in [(12, 6), (13, 7), (14, 8), (19, 9), (20, 10), (17, 4), (18, 5)]:
        if 0 <= y < h:
            img.putpixel((x, y), (70, 66, 62, 255))
    return img


def build_all():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    makers = {
        "sand.png": make_sand,
        "forest_path.png": make_forest_path,
        "forest.png": make_forest,
        "rock.png": make_rock,
    }
    for filename, fn in makers.items():
        img = fn()
        path = os.path.join(OUTPUT_DIR, filename)
        img.save(path)
        print(f"Generated {filename} ({img.size[0]}x{img.size[1]}) -> {path}")


if __name__ == "__main__":
    build_all()
