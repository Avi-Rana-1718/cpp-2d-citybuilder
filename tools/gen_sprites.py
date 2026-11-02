#!/usr/bin/env python3
"""
True 2:1 Isometric Pixel-Art Sprite Generator for TheoTown City-Builder.
Every building features genuine 2:1 slopes, Northwest lighting, and authentic pixel art.
"""
import os
from PIL import Image

OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "..", "assets", "sprites")

def make_grass():
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    for y in range(16):
        hw = (y + 1) * 2 if y < 8 else (16 - y) * 2
        for x in range(16 - hw, 16 + hw):
            # Rich organic meadow texture
            v = ((x * 7 + y * 13) % 23) - 11
            img.putpixel((x, y), (55 + v // 2, 160 + v, 48 + v // 3, 255))
    return img

def make_water():
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    for y in range(16):
        hw = (y + 1) * 2 if y < 8 else (16 - y) * 2
        for x in range(16 - hw, 16 + hw):
            # Translucent wave ripples
            wave = ((x + y * 2) % 6 == 0)
            if wave:
                img.putpixel((x, y), (65, 170, 235, 255)) # Cyan ripple
            else:
                d = ((x * 3 + y * 5) % 15)
                img.putpixel((x, y), (30 + d // 2, 95 + d, 185 + d, 255)) # Deep azure
    return img

def make_road():
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    for y in range(16):
        hw = (y + 1) * 2 if y < 8 else (16 - y) * 2
        for x in range(16 - hw, 16 + hw):
            # Packed gravel/dirt
            noise = ((x * 11 + y * 17) % 19) - 9
            img.putpixel((x, y), (145 + noise, 130 + noise, 110 + noise, 255))
    return img

def make_residental():
    # 32x24 Suburban Cottage (base_y = 8..23, rises 8px up to y=0)
    img = Image.new("RGBA", (32, 24), (0, 0, 0, 0))
    base_y = 8

    # 1. Base Lawn
    for y in range(base_y, 24):
        yr = y - base_y
        hw = (yr + 1) * 2 if yr < 8 else (16 - yr) * 2
        for x in range(16 - hw, 16 + hw):
            v = ((x * 5 + y * 7) % 15)
            img.putpixel((x, y), (60 + v, 165 + v, 50 + v, 255))

    # 2. Isometric Cottage Walls
    # Left Wall (SW facing): from x=6 to x=16
    for x in range(6, 16):
        y_bot = 16 + (x - 6) * 5 // 10
        y_top = y_bot - 8
        for y in range(y_top, y_bot + 1):
            c = (225, 215, 195, 255) if (x + y) % 2 == 0 else (210, 200, 180, 255)
            img.putpixel((x, y), c)

    # Right Wall (SE facing, shadow): from x=16 to x=26
    for x in range(16, 27):
        y_bot = 21 - (x - 16) * 5 // 10
        y_top = y_bot - 8
        for y in range(y_top, y_bot + 1):
            c = (175, 165, 150, 255) if (x + y) % 2 == 0 else (160, 150, 135, 255)
            img.putpixel((x, y), c)

    # 3. Gabled Red Tile Roof (ridge runs from (11, 2) to (21, 7))
    for x in range(5, 27):
        if x <= 16:
            y_eaves = 16 + (x - 6) * 5 // 10 - 8
            y_ridge = 2 + (x - 5) * 5 // 11
        else:
            y_eaves = 21 - (x - 16) * 5 // 10 - 8
            y_ridge = 7 - (x - 16) * 5 // 11

        for y in range(min(y_ridge, y_eaves), max(y_ridge, y_eaves) + 1):
            shade = 205 if x <= 16 else 165
            if (x + y * 2) % 3 == 0:
                shade -= 20
            img.putpixel((x, y), (shade + 20, 50, 40, 255))

    # 4. Chimney (x: 20..22, y: 1..5)
    for cx in [20, 21]:
        for cy in range(2, 6):
            img.putpixel((cx, cy), (140, 60, 50, 255))
    img.putpixel((21, 1), (220, 220, 230, 220)) # Smoke puff

    # 5. Front Door & Windows
    # Wooden front door on left wall (x: 9..10, y: 14..17)
    for dx in [9, 10]:
        for dy in range(14, 18):
            img.putpixel((dx, dy), (120, 70, 35, 255))
    # Window with cyan glass (x: 13..14, y: 12..14)
    img.putpixel((13, 13), (160, 225, 255, 255))
    img.putpixel((14, 13), (190, 240, 255, 255))
    img.putpixel((13, 14), (130, 195, 240, 255))
    img.putpixel((14, 14), (150, 215, 255, 255))

    # Window on right wall (x: 19..20, y: 14..16)
    img.putpixel((19, 14), (140, 200, 240, 255))
    img.putpixel((20, 14), (120, 180, 225, 255))

    return img

def make_commercial():
    # 32x26 Corner Store & Cafe with striped awning (base_y = 10..25, rises 10px up to y=0)
    img = Image.new("RGBA", (32, 26), (0, 0, 0, 0))
    base_y = 10

    # 1. Paved concrete sidewalk base
    for y in range(base_y, 26):
        yr = y - base_y
        hw = (yr + 1) * 2 if yr < 8 else (16 - yr) * 2
        for x in range(16 - hw, 16 + hw):
            c = (145, 150, 155, 255) if (x + y) % 2 == 0 else (135, 140, 145, 255)
            img.putpixel((x, y), c)

    # 2. Isometric Store Walls
    # Left Wall (x: 5..16)
    for x in range(5, 16):
        y_bot = 18 + (x - 5) * 5 // 10
        y_top = y_bot - 11
        for y in range(y_top, y_bot + 1):
            c = (70, 120, 180, 255) if (x + y) % 2 == 0 else (60, 105, 165, 255)
            img.putpixel((x, y), c)

    # Right Wall (x: 16..27)
    for x in range(16, 28):
        y_bot = 23 - (x - 16) * 5 // 10
        y_top = y_bot - 11
        for y in range(y_top, y_bot + 1):
            c = (45, 85, 140, 255) if (x + y) % 2 == 0 else (40, 75, 125, 255)
            img.putpixel((x, y), c)

    # 3. Flat Parapet Roof (x: 5..27)
    for x in range(5, 28):
        if x <= 16:
            y_r = 7 + (x - 5) * 5 // 10
        else:
            y_r = 12 - (x - 16) * 5 // 10
        for y in range(max(1, y_r - 6), y_r + 1):
            c = (190, 195, 200, 255) if (x + y) % 2 == 0 else (175, 180, 185, 255)
            img.putpixel((x, y), c)

    # Rooftop A/C Unit (x: 14..18, y: 3..6)
    for ax in range(14, 18):
        for ay in range(3, 7):
            img.putpixel((ax, ay), (120, 125, 130, 255))
    img.putpixel((15, 4), (80, 85, 90, 255)) # Fan grille

    # 4. Striped Blue/White Awning running along Left Wall (x: 5..16)
    for x in range(5, 17):
        y_awn = 18 + (x - 5) * 5 // 10 - 4
        is_blue = (x % 2 == 0)
        c = (220, 50, 45, 255) if is_blue else (245, 245, 245, 255)
        img.putpixel((x, y_awn), c)
        img.putpixel((x, y_awn + 1), c)

    # 5. Glowing Glass Display Window (x: 8..13, y: 15..18)
    for wx in range(8, 14):
        for wy in range(15, 19):
            img.putpixel((wx, wy), (250, 230, 140, 255)) # Warm warm shoplight
    img.putpixel((10, 16), (255, 245, 190, 255)) # Glare highlight

    return img

def make_industrial():
    # 32x28 Factory with sawtooth roof & smokestack (base_y = 12..27, rises 12px up to y=0)
    img = Image.new("RGBA", (32, 28), (0, 0, 0, 0))
    base_y = 12

    # 1. Concrete ground base
    for y in range(base_y, 28):
        yr = y - base_y
        hw = (yr + 1) * 2 if yr < 8 else (16 - yr) * 2
        for x in range(16 - hw, 16 + hw):
            c = (110, 105, 100, 255) if (x + y) % 2 == 0 else (95, 90, 85, 255)
            img.putpixel((x, y), c)

    # 2. Industrial Brick Warehouse Walls
    for x in range(6, 17):
        y_bot = 20 + (x - 6) * 5 // 10
        y_top = y_bot - 11
        for y in range(y_top, y_bot + 1):
            c = (150, 80, 60, 255) if (x + y) % 2 == 0 else (130, 65, 50, 255)
            img.putpixel((x, y), c)

    for x in range(17, 27):
        y_bot = 25 - (x - 17) * 5 // 10
        y_top = y_bot - 11
        for y in range(y_top, y_bot + 1):
            c = (105, 55, 40, 255) if (x + y) % 2 == 0 else (90, 45, 35, 255)
            img.putpixel((x, y), c)

    # 3. Sawtooth Factory Roof (angled industrial roof)
    for x in range(5, 27):
        if x <= 16:
            y_r = 9 + (x - 5) * 5 // 10
        else:
            y_r = 14 - (x - 16) * 5 // 10
        for y in range(max(2, y_r - 7), y_r + 1):
            c = (165, 160, 155, 255) if (x + y) % 3 == 0 else (140, 135, 130, 255)
            img.putpixel((x, y), c)

    # 4. Brick Smokestack (x: 22..25, y: 1..15)
    for sx in [22, 23, 24]:
        for sy in range(2, 16):
            c = (160, 70, 50, 255) if sx == 22 else (115, 45, 35, 255)
            img.putpixel((sx, sy), c)
    # Steam clouds rising from smokestack
    img.putpixel((23, 1), (240, 240, 245, 230))
    img.putpixel((22, 0), (225, 230, 240, 190))
    img.putpixel((23, 0), (245, 245, 250, 210))

    # 5. Loading Bay Metal Rollup Door (x: 9..14, y: 17..21)
    for lx in range(9, 15):
        for ly in range(17, 22):
            img.putpixel((lx, ly), (180, 185, 195, 255) if ly % 2 == 0 else (140, 145, 155, 255))

    return img

def make_farm():
    # 32x22 Red Barn & Golden Wheat Fields (base_y = 6..21)
    img = Image.new("RGBA", (32, 22), (0, 0, 0, 0))
    base_y = 6

    # 1. Golden wheat crop furrows along 2:1 angle
    for y in range(base_y, 22):
        yr = y - base_y
        hw = (yr + 1) * 2 if yr < 8 else (16 - yr) * 2
        for x in range(16 - hw, 16 + hw):
            # 2:1 diagonal crop stripes
            is_wheat = ((x * 2 - y) % 4 < 2)
            c = (220, 180, 60, 255) if is_wheat else (90, 140, 55, 255)
            img.putpixel((x, y), c)

    # 2. Red Barn (x: 14..25)
    for x in range(14, 21):
        y_bot = 14 + (x - 14) * 5 // 7
        y_top = y_bot - 8
        for y in range(y_top, y_bot + 1):
            img.putpixel((x, y), (180, 45, 35, 255))

    for x in range(21, 27):
        y_bot = 18 - (x - 21) * 5 // 6
        y_top = y_bot - 8
        for y in range(y_top, y_bot + 1):
            img.putpixel((x, y), (135, 30, 25, 255))

    # White Barn Gambrel Roof
    for x in range(13, 27):
        if x <= 20:
            y_r = 6 + (x - 13) * 5 // 7
        else:
            y_r = 11 - (x - 20) * 5 // 7
        for y in range(max(1, y_r - 5), y_r + 1):
            img.putpixel((x, y), (235, 235, 240, 255) if (x + y) % 2 == 0 else (205, 210, 215, 255))

    # White X-brace on barn door (x: 16..19, y: 12..15)
    for dx in range(16, 20):
        img.putpixel((dx, 12), (245, 245, 250, 255))
        img.putpixel((dx, 15), (245, 245, 250, 255))
    img.putpixel((16, 13), (245, 245, 250, 255))
    img.putpixel((19, 14), (245, 245, 250, 255))

    # Silo on left (x: 8..11, y: 4..14)
    for sx in range(8, 12):
        for sy in range(4, 15):
            img.putpixel((sx, sy), (185, 190, 195, 255) if sx <= 9 else (145, 150, 155, 255))
    # Dome top of silo
    img.putpixel((9, 3), (210, 215, 220, 255))
    img.putpixel((10, 3), (170, 175, 180, 255))

    return img

def make_harbour():
    # 32x22 Maritime Port & Dock (base_y = 6..21)
    img = Image.new("RGBA", (32, 22), (0, 0, 0, 0))
    base_y = 6

    # Half water, half wooden pier
    for y in range(base_y, 22):
        yr = y - base_y
        hw = (yr + 1) * 2 if yr < 8 else (16 - yr) * 2
        for x in range(16 - hw, 16 + hw):
            if x <= 16: # Wooden pier
                plank = ((x + y * 2) % 3 == 0)
                c = (130, 95, 65, 255) if plank else (155, 115, 80, 255)
            else: # Water
                c = (35, 105, 190, 255) if (x + y) % 4 == 0 else (25, 85, 165, 255)
            img.putpixel((x, y), c)

    # Mooring bollards along dock edge (x=16)
    for by in [10, 16]:
        img.putpixel((16, by), (40, 40, 45, 255))
        img.putpixel((16, by - 1), (60, 60, 65, 255))

    # Cargo Shipping Containers stacked on pier (x: 5..14)
    # Blue container
    for cx in range(6, 12):
        for cy in range(7, 12):
            img.putpixel((cx, cy), (45, 110, 200, 255) if cx <= 8 else (30, 80, 155, 255))
    # Orange container
    for cx in range(8, 14):
        for cy in range(11, 16):
            img.putpixel((cx, cy), (230, 105, 30, 255) if cx <= 10 else (190, 75, 20, 255))

    return img

def make_park():
    # 32x24 Municipal Nature Park with 3D Oak Tree & Fountain (base_y = 8..23)
    img = Image.new("RGBA", (32, 24), (0, 0, 0, 0))
    base_y = 8

    # 1. Park Lawn
    for y in range(base_y, 24):
        yr = y - base_y
        hw = (yr + 1) * 2 if yr < 8 else (16 - yr) * 2
        for x in range(16 - hw, 16 + hw):
            v = ((x * 7 + y * 11) % 17)
            img.putpixel((x, y), (50 + v, 170 + v, 45 + v, 255))

    # 2. Circular Cobblestone Plaza & Water Fountain (center: 21, 16)
    for dy in range(-2, 3):
        for dx in range(-4, 5):
            if (dx/4.0)**2 + (dy/2.0)**2 <= 1.0:
                img.putpixel((21 + dx, 16 + dy), (175, 175, 180, 255))
    # Blue fountain pool
    img.putpixel((21, 16), (60, 180, 245, 255))
    img.putpixel((20, 16), (40, 150, 225, 255))
    img.putpixel((21, 15), (180, 230, 255, 255)) # Water splash

    # 3. Wooden Park Bench (x: 18..20, y: 19..20)
    img.putpixel((19, 19), (130, 80, 45, 255))
    img.putpixel((20, 19), (130, 80, 45, 255))
    img.putpixel((19, 20), (90, 55, 30, 255))

    # 4. 3D Oak Tree with Foliage Canopy (Trunk at x: 10, y: 13..18)
    for ty in range(13, 19):
        img.putpixel((10, ty), (100, 60, 35, 255))
        img.putpixel((11, ty), (80, 45, 25, 255))

    # Spherical isometric green foliage (center: 10, 8)
    for dy in range(-6, 6):
        for dx in range(-7, 8):
            if (dx/7.0)**2 + (dy/5.5)**2 <= 1.0:
                px = 10 + dx
                py = 8 + dy
                if 0 <= px < 32 and 0 <= py < 24:
                    # Northwest sun lighting on canopy
                    if dx <= 0 and dy <= 0:
                        c = (70, 185, 55, 255) # Bright sunlit foliage
                    elif dx > 1 or dy > 1:
                        c = (30, 105, 35, 255) # Shadow underside
                    else:
                        c = (45, 145, 45, 255) # Midtone leaf
                    if (px + py * 2) % 3 == 0:
                        c = (min(255, c[0] + 15), min(255, c[1] + 15), min(255, c[2] + 15), 255)
                    img.putpixel((px, py), c)

    return img

def make_school():
    # 32x26 Elementary School with bell tower (base_y = 10..25, rises 10px up to y=0)
    img = Image.new("RGBA", (32, 26), (0, 0, 0, 0))
    base_y = 10

    # 1. School Lawn & Front Pathway
    for y in range(base_y, 26):
        yr = y - base_y
        hw = (yr + 1) * 2 if yr < 8 else (16 - yr) * 2
        for x in range(16 - hw, 16 + hw):
            img.putpixel((x, y), (65, 160, 55, 255))

    # 2. Isometric Brick School Walls
    for x in range(5, 16):
        y_bot = 18 + (x - 5) * 5 // 10
        y_top = y_bot - 9
        for y in range(y_top, y_bot + 1):
            c = (180, 60, 50, 255) if (x + y) % 2 == 0 else (160, 50, 40, 255)
            img.putpixel((x, y), c)

    for x in range(16, 27):
        y_bot = 23 - (x - 16) * 5 // 10
        y_top = y_bot - 9
        for y in range(y_top, y_bot + 1):
            c = (140, 45, 38, 255) if (x + y) % 2 == 0 else (120, 38, 30, 255)
            img.putpixel((x, y), c)

    # 3. Gabled Slate Roof
    for x in range(4, 28):
        if x <= 16:
            y_r = 9 + (x - 4) * 5 // 12
        else:
            y_r = 14 - (x - 16) * 5 // 11
        for y in range(max(2, y_r - 6), y_r + 1):
            img.putpixel((x, y), (65, 80, 105, 255) if x <= 16 else (45, 60, 85, 255))

    # 4. Central Bell Tower & Golden School Bell (x: 14..18, y: 1..8)
    for bx in range(14, 18):
        for by in range(3, 9):
            img.putpixel((bx, by), (235, 235, 240, 255) if bx <= 15 else (195, 200, 205, 255))
    # Golden Bell inside tower opening
    img.putpixel((15, 6), (250, 210, 35, 255))
    img.putpixel((16, 6), (240, 195, 25, 255))
    # Tower Spire Peak
    img.putpixel((15, 2), (180, 50, 40, 255))
    img.putpixel((16, 2), (180, 50, 40, 255))
    img.putpixel((15, 1), (250, 215, 40, 255)) # Finial

    # 5. Classroom Windows (White cross-muntins & blue glass)
    for (wx, wy) in [(8, 14), (12, 16), (20, 16), (23, 14)]:
        img.putpixel((wx, wy), (180, 235, 255, 255))
        img.putpixel((wx + 1, wy), (140, 205, 245, 255))

    # Arched wooden entrance (x: 15..17, y: 18..21)
    img.putpixel((15, 19), (110, 65, 35, 255))
    img.putpixel((16, 19), (110, 65, 35, 255))
    img.putpixel((15, 20), (80, 45, 25, 255))
    img.putpixel((16, 20), (80, 45, 25, 255))

    return img

def make_church():
    # 32x30 Gothic Cathedral & Steeple Spire (base_y = 14..29, rises 14px up to y=0)
    img = Image.new("RGBA", (32, 30), (0, 0, 0, 0))
    base_y = 14

    # 1. Stone Churchyard & Lawn Base
    for y in range(base_y, 30):
        yr = y - base_y
        hw = (yr + 1) * 2 if yr < 8 else (16 - yr) * 2
        for x in range(16 - hw, 16 + hw):
            c = (145, 150, 155, 255) if (x + y) % 3 == 0 else (55, 145, 50, 255)
            img.putpixel((x, y), c)

    # 2. Ashlar Stone Cathedral Walls (x: 6..26)
    for x in range(6, 17):
        y_bot = 22 + (x - 6) * 5 // 10
        y_top = y_bot - 11
        for y in range(y_top, y_bot + 1):
            c = (170, 175, 185, 255) if (x + y) % 2 == 0 else (150, 155, 165, 255)
            img.putpixel((x, y), c)

    for x in range(17, 27):
        y_bot = 27 - (x - 17) * 5 // 10
        y_top = y_bot - 11
        for y in range(y_top, y_bot + 1):
            c = (130, 135, 145, 255) if (x + y) % 2 == 0 else (115, 120, 130, 255)
            img.putpixel((x, y), c)

    # 3. Steep Gothic Roof
    for x in range(5, 27):
        if x <= 16:
            y_r = 11 + (x - 5) * 5 // 11
        else:
            y_r = 16 - (x - 16) * 5 // 10
        for y in range(max(3, y_r - 7), y_r + 1):
            img.putpixel((x, y), (85, 80, 95, 255) if x <= 16 else (60, 55, 70, 255))

    # 4. Belfry Spire Tower (x: 8..14, rises to y=0!)
    for tx in range(9, 14):
        for ty in range(5, 16):
            img.putpixel((tx, ty), (180, 185, 195, 255) if tx <= 11 else (140, 145, 155, 255))
    # Steeple Pyramidal Spire (y: 2..5)
    for sy in range(2, 6):
        sw = max(0, sy - 2)
        for sx in range(11 - sw, 11 + sw + 1):
            img.putpixel((sx, sy), (130, 135, 145, 255))

    # Golden Crucifix atop Steeple (y: 0..3)
    img.putpixel((11, 0), (255, 225, 45, 255))
    img.putpixel((10, 1), (255, 225, 45, 255))
    img.putpixel((11, 1), (255, 235, 65, 255))
    img.putpixel((12, 1), (255, 225, 45, 255))
    img.putpixel((11, 2), (245, 215, 40, 255))

    # 5. Stained Glass Rose Window on Nave facade (center: 20, 18)
    img.putpixel((20, 17), (240, 50, 60, 255))  # Ruby
    img.putpixel((19, 18), (50, 140, 245, 255)) # Sapphire
    img.putpixel((21, 18), (245, 210, 40, 255)) # Gold
    img.putpixel((20, 19), (60, 215, 90, 255))  # Emerald

    # Arched Gothic Wooden Entrance (x: 10..12, y: 17..21)
    for ex in [10, 11]:
        for ey in range(17, 22):
            img.putpixel((ex, ey), (90, 55, 30, 255) if ey >= 18 else (195, 200, 210, 255))

    return img

def build_all():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    makers = {
        "grass.png": make_grass,
        "water.png": make_water,
        "road.png": make_road,
        "residental.png": make_residental,
        "commercial.png": make_commercial,
        "industrial.png": make_industrial,
        "farm.png": make_farm,
        "harbour.png": make_harbour,
        "park.png": make_park,
        "school.png": make_school,
        "church.png": make_church,
    }
    for filename, fn in makers.items():
        img = fn()
        path = os.path.join(OUTPUT_DIR, filename)
        img.save(path)
        print(f"Generated {filename} ({img.size[0]}x{img.size[1]}) -> {path}")

if __name__ == "__main__":
    build_all()
