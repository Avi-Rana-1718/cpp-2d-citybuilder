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


def summer_canopy(shade):
    def paint(x, y, t, lit):
        g = shade + (22 if lit else 0) + ((x * 3 + y * 5) % 7) - 3
        return (28 + (8 if lit else 0), g, 36)
    return paint


def autumn_canopy(base_rgb):
    # Deciduous autumn colours, brighter on the lit north-west side
    def paint(x, y, t, lit):
        n = ((x * 3 + y * 5) % 7) - 3
        boost = 28 if lit else 0
        r, g, b = base_rgb
        return (min(255, r + boost + n * 3), min(255, g + boost // 2 + n * 2), min(255, b + n))
    return paint


def winter_canopy(shade):
    # Dark conifer green with snow settled on the top and the lit side
    def paint(x, y, t, lit):
        if t < 0.22 or (lit and (x + y) % 3 != 0):
            v = ((x * 5 + y * 3) % 5) * 3
            return (226 + v, 232 + v, 242 + v // 2)
        g = shade - 15 + ((x * 3 + y * 5) % 7) - 3
        return (24, g, 38)
    return paint


def draw_tree(img, cx, base, height, radius, shade, paint=None):
    if paint is None:
        paint = summer_canopy(shade)
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
            img.putpixel((x, y), (*paint(x, y, t, lit), 255))


# Tree layout shared by every season: (cx, base offset, height, radius, shade)
TREES = [(10, 10, 13, 5, 105), (21, 10, 15, 5, 98), (15, 6, 14, 6, 112), (6, 5, 10, 4, 100), (25, 5, 11, 4, 108)]
AUTUMN_COLOURS = [(205, 105, 30), (185, 55, 30), (215, 160, 35), None, (200, 85, 25)]  # None = evergreen


def make_forest_season(season):
    h = 32
    img = Image.new("RGBA", (32, h), (0, 0, 0, 0))
    # Back-to-front so nearer trees overlap farther ones
    for k, (cx, off, height, radius, shade) in enumerate(TREES):
        if season == "autumn" and AUTUMN_COLOURS[k] is not None:
            paint = autumn_canopy(AUTUMN_COLOURS[k])
        elif season == "winter":
            paint = winter_canopy(shade)
        else:
            paint = summer_canopy(shade)
        draw_tree(img, cx, h - off, height, radius, shade, paint)
    return img


def make_forest():
    return make_forest_season("summer")


def make_forest_autumn():
    return make_forest_season("autumn")


def make_forest_winter():
    return make_forest_season("winter")


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
        "forest_autumn.png": make_forest_autumn,
        "forest_winter.png": make_forest_winter,
        "rock.png": make_rock,
    }
    for filename, fn in makers.items():
        img = fn()
        path = os.path.join(OUTPUT_DIR, filename)
        img.save(path)
        print(f"Generated {filename} ({img.size[0]}x{img.size[1]}) -> {path}")


if __name__ == "__main__":
    build_all()
