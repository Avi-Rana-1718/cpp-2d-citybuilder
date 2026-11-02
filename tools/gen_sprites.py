#!/usr/bin/env python3
"""
Generate crisp 32x16 (and isometric height) pixel-art sprites for TheoTown city-builder:
- commercial.png
- industrial.png
- farm.png
- school.png
- church.png
- park.png
- harbour.png
- water.png
"""

import os
from PIL import Image, ImageDraw

OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "..", "assets", "sprites")

def get_base_diamond():
    """Returns a base 32x16 transparent image with an isometric grass/ground diamond."""
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    # Diamond mask
    for y in range(16):
        if y < 8:
            hw = (y + 1) * 2
        else:
            hw = (16 - y) * 2
        for x in range(16 - hw, 16 + hw):
            # Ground shading
            g = 140 + ((x + y * 3) % 15)
            img.putpixel((x, y), (50, g, 40, 255))
    return img

def make_commercial():
    # 32x18 with storefront & awning
    img = Image.new("RGBA", (32, 18), (0, 0, 0, 0))
    # Ground diamond at bottom (y 2..17)
    for y in range(2, 18):
        y_rel = y - 2
        hw = (y_rel + 1) * 2 if y_rel < 8 else (16 - y_rel) * 2
        for x in range(16 - hw, 16 + hw):
            img.putpixel((x, y), (120, 125, 130, 255)) # Paved concrete base

    # Shop building cube
    # Left wall (x: 8..15, y: 3..12)
    for x in range(8, 16):
        for y in range(4, 12):
            img.putpixel((x, y), (60, 100, 160, 255)) # Blue commercial wall
    # Right wall (x: 16..23, y: 4..12)
    for x in range(16, 24):
        for y in range(4, 12):
            img.putpixel((x, y), (45, 80, 135, 255)) # Shaded right wall

    # Shop roof
    for x in range(10, 22):
        for y in range(1, 5):
            if (x + y) % 2 == 0:
                img.putpixel((x, y), (190, 195, 205, 255))
            else:
                img.putpixel((x, y), (170, 175, 185, 255))

    # Striped Red & White awning (y: 6..8, x: 9..22)
    for x in range(9, 23):
        color = (220, 50, 45, 255) if (x // 2) % 2 == 0 else (245, 245, 245, 255)
        img.putpixel((x, 7), color)
        img.putpixel((x, 8), color)

    # Glass display windows (y: 9..11)
    for x in range(10, 14):
        img.putpixel((x, 10), (160, 225, 255, 255))
        img.putpixel((x, 11), (130, 195, 240, 255))
    for x in range(18, 22):
        img.putpixel((x, 10), (160, 225, 255, 255))
        img.putpixel((x, 11), (130, 195, 240, 255))

    # Door
    img.putpixel((15, 10), (220, 180, 60, 255))
    img.putpixel((16, 10), (220, 180, 60, 255))
    img.putpixel((15, 11), (180, 140, 40, 255))
    img.putpixel((16, 11), (180, 140, 40, 255))

    return img

def make_industrial():
    # 32x20 Factory with smokestack
    img = Image.new("RGBA", (32, 20), (0, 0, 0, 0))
    # Concrete base
    for y in range(4, 20):
        y_rel = y - 4
        hw = (y_rel + 1) * 2 if y_rel < 8 else (16 - y_rel) * 2
        for x in range(16 - hw, 16 + hw):
            img.putpixel((x, y), (90, 85, 80, 255))

    # Factory brick walls (x: 7..24, y: 7..15)
    for x in range(8, 24):
        for y in range(7, 15):
            c = (140, 70, 50, 255) if (x + y) % 3 != 0 else (120, 55, 40, 255)
            img.putpixel((x, y), c)

    # Sawtooth industrial roof
    for x in range(8, 24):
        for y in range(4, 8):
            if (x % 5) >= (y - 3):
                img.putpixel((x, y), (80, 85, 95, 255))

    # Smokestack chimney (x: 20..22, y: 1..7)
    for y in range(2, 7):
        img.putpixel((20, y), (180, 80, 50, 255))
        img.putpixel((21, y), (150, 65, 40, 255))
    # White smoke puff
    img.putpixel((21, 1), (220, 220, 230, 200))
    img.putpixel((22, 0), (240, 240, 250, 230))
    img.putpixel((23, 1), (210, 215, 225, 180))

    # Factory loading bay door
    for x in range(12, 17):
        for y in range(11, 15):
            img.putpixel((x, y), (50, 55, 60, 255))

    return img

def make_farm():
    # 32x16 Agricultural crop field with wheat & mini barn
    img = get_base_diamond()
    # Golden tilled soil stripes
    for y in range(1, 15):
        hw = (y + 1) * 2 if y < 8 else (16 - y) * 2
        for x in range(16 - hw + 2, 16 + hw - 2):
            if (x + y) % 3 == 0:
                img.putpixel((x, y), (185, 145, 65, 255)) # Wheat yellow
            elif (x + y) % 3 == 1:
                img.putpixel((x, y), (120, 85, 35, 255)) # Dark tilled loam
            else:
                img.putpixel((x, y), (80, 160, 50, 255)) # Sprouting green crop

    # Mini red barn (x: 19..25, y: 3..9)
    for x in range(20, 25):
        for y in range(4, 9):
            img.putpixel((x, y), (180, 35, 30, 255))
    # Barn roof
    for x in range(20, 25):
        img.putpixel((x, 3), (220, 210, 200, 255))
    # Barn door
    img.putpixel((22, 7), (240, 235, 220, 255))
    img.putpixel((22, 8), (240, 235, 220, 255))

    return img

def is_in_2x2_base(x, y, base_y):
    """Checks if pixel (x, y) is inside the 2x2 contiguous isometric base diamond."""
    for ox, oy in [(16, base_y + 8), (32, base_y + 16), (16, base_y + 24), (32, base_y + 32)]:
        dx = abs(x - ox)
        dy = abs(y - oy)
        if (dx / 16.0) + (dy / 8.0) <= 1.0001:
            return True
    return False

def make_school():
    # 48x64 Multi-tile 2x2 Brick Elementary School with Bell Tower, Wings & Courtyard
    # base_y = 24. Base diamond spans from y=24 to y=64 (48x40 footprint).
    img = Image.new("RGBA", (48, 64), (0, 0, 0, 0))
    base_y = 24

    # 1. Ground Foundation (Lawn & Courtyard Path)
    for y in range(base_y, 64):
        for x in range(48):
            if is_in_2x2_base(x, y, base_y):
                # Paved stone pathway leading through center
                if 20 <= x <= 28 and y >= 44:
                    c = (195, 190, 180, 255) if (x + y) % 2 == 0 else (175, 170, 160, 255)
                else:
                    g = 135 + ((x * 3 + y * 7) % 25)
                    c = (55, g, 45, 255) # Manicured school lawn
                img.putpixel((x, y), c)

    # Courtyard border shrubs
    for (sx, sy) in [(18, 52), (19, 53), (29, 52), (30, 53), (12, 40), (36, 40)]:
        if 0 <= sx < 48 and 0 <= sy < 64:
            img.putpixel((sx, sy), (30, 110, 35, 255))
            img.putpixel((sx, sy - 1), (45, 140, 50, 255))

    # Flagpole on front lawn (x=33, y=42..54)
    for fy in range(44, 55):
        img.putpixel((34, fy), (200, 205, 215, 255))
    # Flag banner
    for fx in range(35, 39):
        img.putpixel((fx, 45), (220, 50, 40, 255))
        img.putpixel((fx, 46), (50, 100, 220, 255))

    # 2. Main Central School Hall (Brick structure: x:14..34, y:24..45)
    for y in range(26, 45):
        for x in range(14, 34):
            # Brick texture shading
            base_r = 175 if (x + y) % 2 == 0 else 155
            base_g = 55 if (x + y) % 2 == 0 else 45
            base_b = 45 if (x + y) % 2 == 0 else 38
            # Right side isometric wall shading
            if x >= 24:
                base_r = int(base_r * 0.85)
                base_g = int(base_g * 0.85)
                base_b = int(base_b * 0.85)
            img.putpixel((x, y), (base_r, base_g, base_b, 255))

    # 3. Left Wing (x: 7..14, y: 28..42)
    for y in range(29, 43):
        for x in range(7, 15):
            c = (165, 50, 42, 255) if (x + y) % 2 == 0 else (145, 42, 35, 255)
            img.putpixel((x, y), c)

    # 4. Right Wing (x: 34..41, y: 24..38)
    for y in range(25, 39):
        for x in range(34, 42):
            c = (140, 40, 35, 255) if (x + y) % 2 == 0 else (120, 35, 30, 255)
            img.putpixel((x, y), c)

    # 5. Gabled Roofs
    # Central Hall Roof (x: 13..35, y: 16..26)
    for y in range(16, 27):
        for x in range(13, 35):
            dy = y - 16
            if abs(x - 24) <= dy * 2 + 3:
                r_col = (70, 85, 110, 255) if x < 24 else (50, 65, 90, 255)
                if (x + y) % 3 == 0:
                    r_col = (r_col[0] + 15, r_col[1] + 15, r_col[2] + 15, 255)
                img.putpixel((x, y), r_col)

    # Left Wing Roof (x: 6..15, y: 24..29)
    for y in range(24, 30):
        for x in range(6, 15):
            img.putpixel((x, y), (65, 80, 105, 255))

    # Right Wing Roof (x: 33..42, y: 20..25)
    for y in range(20, 26):
        for x in range(33, 42):
            img.putpixel((x, y), (45, 60, 85, 255))

    # 6. Central Bell / Clock Tower (x: 21..27, y: 4..16)
    for y in range(6, 17):
        for x in range(21, 27):
            c = (235, 235, 240, 255) if x <= 23 else (195, 200, 205, 255)
            img.putpixel((x, y), c)

    # Golden School Bell in Tower Arch (x: 23..24, y: 11..13)
    img.putpixel((23, 11), (245, 200, 30, 255))
    img.putpixel((24, 11), (255, 215, 45, 255))
    img.putpixel((23, 12), (230, 185, 25, 255))
    img.putpixel((24, 12), (245, 200, 30, 255))

    # Tower Spire (y: 2..6)
    for y in range(3, 7):
        hw = (y - 2)
        for x in range(24 - hw, 24 + hw):
            img.putpixel((x, y), (180, 45, 40, 255))
    img.putpixel((24, 2), (245, 215, 40, 255)) # Golden finial

    # 7. Classroom Windows (White frame, sky blue glass)
    win_coords = [
        # Central Upper floor
        (16, 28), (19, 28), (28, 28), (31, 28),
        # Central Lower floor
        (16, 35), (19, 35), (28, 35), (31, 35),
        # Left Wing
        (9, 34), (12, 34),
        # Right Wing
        (36, 30), (39, 30)
    ]
    for (wx, wy) in win_coords:
        img.putpixel((wx, wy), (160, 220, 255, 255))
        img.putpixel((wx + 1, wy), (190, 235, 255, 255))
        img.putpixel((wx, wy + 1), (120, 180, 230, 255))
        img.putpixel((wx + 1, wy + 1), (140, 195, 240, 255))

    # 8. Main Arched Entrance (x: 22..25, y: 39..44)
    for y in range(39, 44):
        for x in range(22, 26):
            if y == 39:
                img.putpixel((x, y), (210, 210, 215, 255)) # Stone arch
            else:
                img.putpixel((x, y), (90, 50, 30, 255)) # Rich mahogany double doors
    img.putpixel((23, 42), (240, 200, 40, 255)) # Brass door handles
    img.putpixel((24, 42), (240, 200, 40, 255))

    return img

def make_church():
    # 48x70 Multi-tile 2x2 Gothic Cathedral with Stone Nave, Rose Window & Grand Spire
    # base_y = 28. Base diamond spans from y=28 to y=68 (48x40 footprint).
    img = Image.new("RGBA", (48, 70), (0, 0, 0, 0))
    base_y = 28

    # 1. Ground Foundation (Stone Flagstones, Cypress Trees, Lawn)
    for y in range(base_y, 68):
        for x in range(48):
            if is_in_2x2_base(x, y, base_y):
                # Flagstone courtyard around cathedral
                if (18 <= x <= 32 and y >= 44) or (x + y * 2) % 9 == 0:
                    c = (150, 155, 160, 255) if (x + y) % 2 == 0 else (130, 135, 140, 255)
                else:
                    g = 120 + ((x * 4 + y * 5) % 20)
                    c = (50, g, 45, 255) # Churchyard grass
                img.putpixel((x, y), c)

    # Churchyard ornamental cypress/yew trees
    for (tx, ty) in [(6, 44), (40, 36), (36, 56)]:
        for h in range(7):
            w = max(0, 3 - h // 2)
            for dx in range(-w, w + 1):
                img.putpixel((tx + dx, ty - h), (25, 80 + h * 5, 35, 255))

    # 2. Cathedral Nave & Transept Walls (x: 14..38, y: 32..50)
    for y in range(32, 50):
        for x in range(14, 38):
            # Heavy ashlar stone blocks
            base_val = 165 if (x + y) % 2 == 0 else 145
            if (x % 3 == 0) and (y % 2 == 0):
                base_val -= 25 # Mortar groove
            # Isometric wall lighting
            if x >= 26:
                base_val = int(base_val * 0.85)
            img.putpixel((x, y), (base_val, base_val + 2, base_val + 6, 255))

    # Flying Buttresses (external structural pillars)
    for bx in [14, 20, 32, 38]:
        for by in range(36, 50):
            img.putpixel((bx, by), (180, 185, 195, 255) if bx < 26 else (135, 140, 150, 255))

    # 3. Steep Pitched Gothic Nave Roof (x: 13..39, y: 20..32)
    for y in range(20, 33):
        for x in range(13, 39):
            dy = y - 20
            if abs(x - 26) <= dy * 2 + 2:
                # Slate shingles
                s = 70 if x < 26 else 50
                if (x + y * 2) % 3 == 0:
                    s += 12
                img.putpixel((x, y), (s + 5, s, s + 15, 255))

    # 4. Majestic Westwork Bell Tower & Octagonal Spire (x: 10..18, y: 0..46)
    # Tower base & belfry (y: 14..36)
    for y in range(14, 36):
        for x in range(10, 18):
            c = (175, 180, 190, 255) if x <= 13 else (140, 145, 155, 255)
            img.putpixel((x, y), c)

    # Belfry louvres (openings for bells)
    for y in [20, 21, 23, 24]:
        img.putpixel((12, y), (30, 30, 35, 255))
        img.putpixel((15, y), (20, 20, 25, 255))

    # Octagonal Gothic Spire (y: 2..14)
    for y in range(3, 14):
        sw = max(0, int((y - 2) * 0.65))
        for x in range(14 - sw, 14 + sw + 1):
            c = (135, 140, 150, 255) if x <= 14 else (105, 110, 120, 255)
            img.putpixel((x, y), c)

    # Golden Crucifix at Apex of Spire (y: 0..4)
    img.putpixel((14, 0), (255, 220, 40, 255))
    img.putpixel((13, 1), (255, 220, 40, 255))
    img.putpixel((14, 1), (255, 230, 60, 255))
    img.putpixel((15, 1), (255, 220, 40, 255))
    img.putpixel((14, 2), (245, 210, 35, 255))

    # 5. Stained Glass Rose Window (Large circular stained glass window)
    cx, cy = 26, 36
    for dy in range(-4, 5):
        for dx in range(-4, 5):
            dist_sq = dx * dx + dy * dy
            if dist_sq <= 15:
                # Stone tracery rim
                if dist_sq >= 12:
                    img.putpixel((cx + dx, cy + dy), (200, 205, 215, 255))
                # Brilliant jewel-toned stained glass petals
                elif abs(dx) == abs(dy):
                    img.putpixel((cx + dx, cy + dy), (245, 210, 40, 255)) # Gold
                elif dx == 0 or dy == 0:
                    img.putpixel((cx + dx, cy + dy), (235, 45, 55, 255))  # Ruby Red
                elif dx > 0:
                    img.putpixel((cx + dx, cy + dy), (45, 140, 245, 255)) # Cobalt Blue
                else:
                    img.putpixel((cx + dx, cy + dy), (60, 215, 90, 255))  # Emerald Green

    # 6. Flanking Lancet Stained Glass Windows
    for (wx, wy) in [(18, 38), (34, 38)]:
        for h in range(5):
            img.putpixel((wx, wy + h), (180, 80, 220, 255) if h % 2 == 0 else (60, 170, 240, 255))

    # 7. Grand Gothic Portal & Double Wooden Entrance (x: 23..29, y: 44..49)
    for y in range(43, 50):
        for x in range(23, 30):
            if y == 43 and abs(x - 26) <= 2:
                img.putpixel((x, y), (215, 215, 225, 255)) # Gothic carved archway
            elif y >= 44:
                img.putpixel((x, y), (85, 45, 25, 255)) # Heavy carved oak doors
    # Iron decorative hinge straps
    img.putpixel((24, 46), (40, 40, 45, 255))
    img.putpixel((28, 46), (30, 30, 35, 255))
    img.putpixel((24, 48), (40, 40, 45, 255))
    img.putpixel((28, 48), (30, 30, 35, 255))

    return img

def make_park():
    # 32x18 Park with lush oak tree, bench, and water fountain
    img = Image.new("RGBA", (32, 18), (0, 0, 0, 0))
    for y in range(2, 18):
        y_rel = y - 2
        hw = (y_rel + 1) * 2 if y_rel < 8 else (16 - y_rel) * 2
        for x in range(16 - hw, 16 + hw):
            img.putpixel((x, y), (60, 180, 55, 255)) # Vibrant grass

    # Gravel circular walking path
    for y in range(6, 14):
        for x in range(10, 22):
            if (x - 16)**2 + (y - 10)**2 in [12, 13, 14, 15, 16]:
                img.putpixel((x, y), (210, 195, 170, 255))

    # Central water fountain (x: 14..18, y: 8..11)
    img.putpixel((15, 9), (70, 190, 250, 255))
    img.putpixel((16, 9), (90, 210, 255, 255))
    img.putpixel((15, 10), (50, 160, 230, 255))
    img.putpixel((16, 10), (70, 190, 250, 255))
    # Fountain stone basin
    img.putpixel((14, 9), (190, 195, 200, 255))
    img.putpixel((17, 9), (190, 195, 200, 255))
    img.putpixel((14, 10), (170, 175, 180, 255))
    img.putpixel((17, 10), (170, 175, 180, 255))

    # Oak Tree (x: 6..12, y: 1..8)
    # Trunk
    img.putpixel((8, 8), (110, 75, 45, 255))
    img.putpixel((9, 8), (95, 60, 35, 255))
    img.putpixel((8, 7), (110, 75, 45, 255))
    # Foliage canopy
    for x in range(5, 12):
        for y in range(2, 7):
            if (x - 8)**2 + (y - 4)**2 <= 8:
                c = (35, 145, 40, 255) if (x + y) % 2 == 0 else (50, 170, 55, 255)
                img.putpixel((x, y), c)

    # Flowerbeds (red & yellow pixels)
    img.putpixel((21, 8), (240, 60, 60, 255))
    img.putpixel((22, 9), (250, 210, 50, 255))
    img.putpixel((20, 11), (230, 80, 190, 255))

    return img

def make_harbour():
    # 32x18 Wooden pier dock over water with boat
    img = Image.new("RGBA", (32, 18), (0, 0, 0, 0))
    # Blue water base
    for y in range(2, 18):
        y_rel = y - 2
        hw = (y_rel + 1) * 2 if y_rel < 8 else (16 - y_rel) * 2
        for x in range(16 - hw, 16 + hw):
            img.putpixel((x, y), (35, 115, 195, 255))

    # Wooden pier dock (x: 8..24, y: 7..12)
    for x in range(9, 23):
        for y in range(7, 12):
            c = (165, 120, 75, 255) if x % 2 == 0 else (145, 100, 60, 255)
            img.putpixel((x, y), c)

    # Mooring bollards
    img.putpixel((10, 6), (70, 70, 75, 255))
    img.putpixel((16, 6), (70, 70, 75, 255))
    img.putpixel((21, 6), (70, 70, 75, 255))

    # Moored white boat (x: 13..21, y: 3..6)
    for x in range(14, 20):
        img.putpixel((x, 4), (240, 240, 245, 255))
        img.putpixel((x, 5), (210, 215, 220, 255))
    img.putpixel((13, 4), (200, 60, 50, 255)) # Bow red accent
    img.putpixel((20, 4), (180, 185, 190, 255))

    return img

def make_water():
    # 32x16 Sparkling blue isometric water diamond
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    for y in range(16):
        hw = (y + 1) * 2 if y < 8 else (16 - y) * 2
        for x in range(16 - hw, 16 + hw):
            # Water wave texture
            wave = (x * 2 + y * 3) % 7
            if wave == 0:
                c = (75, 175, 245, 255) # Light crest
            elif wave <= 2:
                c = (45, 135, 215, 255) # Mid blue
            else:
                c = (30, 105, 185, 255) # Deep water
            img.putpixel((x, y), c)
    return img

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    sprites = {
        "commercial.png": make_commercial(),
        "industrial.png": make_industrial(),
        "farm.png": make_farm(),
        "school.png": make_school(),
        "church.png": make_church(),
        "park.png": make_park(),
        "harbour.png": make_harbour(),
        "water.png": make_water(),
    }

    for name, img in sprites.items():
        path = os.path.join(OUTPUT_DIR, name)
        img.save(path, format="PNG")
        print(f"Generated {name} ({img.size[0]}x{img.size[1]}) -> {path}")

if __name__ == "__main__":
    main()
